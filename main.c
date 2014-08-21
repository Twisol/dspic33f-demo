#include <stddef.h>        /* NULL */
#include <stdint.h>        /* /u?int[8|16|32|64]_t */
#include <stdbool.h>       /* bool */
#include <string.h>        /* memcpy */

#include <xc.h>

#include "EventBus.h"
#include "EventTypes.h"
#include "RawComm.h"
#include "Defer.h"

#include "drivers/LCD.h"
#include "drivers/LED.h"
#include "drivers/PushButtons.h"
#include "drivers/UART.h"
#include "drivers/SD.h"


uint8_t digit_to_char[] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};
// Only supports base 2 through base 16
uint8_t* uitoab(uint16_t num, uint16_t base, uint8_t* buf, uint16_t len) {
  uint8_t* itr = buf + len-1;

  *itr = '\0';

  do {
    itr -= 1;

    if (itr < buf) {
      itr = 0;
      break;
    }

    int digit = num % base;
    num /= base;
    *itr = digit_to_char[digit];
  } while (num > 0);

  return itr;
}

uint8_t* uitoa(uint16_t num, uint8_t* buf, uint16_t len) {
  return uitoab(num, 10, buf, len);
}


typedef struct AppState {
  EventBus eventBus;
  RawComm dev;

  int count;
} AppState;


static AppState app;

RawComm* _InterruptGetRawComm() {
  return &app.dev;
}

void buttonHandler(AppState* self) {
  // Display the current number
  uint8_t buf[17];
  uint8_t* res = uitoa(self->count, buf, 17);
  if (res != 0) {
    LCD_PutString(res);

    UART_PutString(&self->dev.uart, res, 16-(res-buf));
    UART_PutString(&self->dev.uart, "\r\n", 2);
  }
}

void timerHandler(AppState* self) {
  // Display the current number
  self->count += 1;

  uint8_t bits = self->count << 2;
  if (SD_CardDetected(&self->dev.sd)) {
    bits = bits | 0x01;
  } else {
    bits = bits &~ 0x01;
  }

  if (SD_WriteProtected(&self->dev.sd)) {
    bits = bits | 0x02;
  } else {
    bits = bits &~ 0x02;
  }

  LED_Toggle(bits);

  Defer_Set(&self->dev.defer, 500, &app.eventBus, EVT_TIMER1, NULL);
}

void inputHandler(AppState* self) {
  // Act as an echo server
  uint8_t str[32];
  uint16_t bytesRead = UART_GetString(&self->dev.uart, str, 32);

  UART_PutString(&self->dev.uart, str, bytesRead);

  // If there were more than 32 characters,
  // come back later to do the rest.
  if (UART_GetCount(&self->dev.uart) > 0) {
    EventBus_Signal(&self->eventBus, EVT_UART);
  }
}

void print(uint8_t* cstr) {
  uint16_t len = 0;
  while (cstr[len] != '\0') {
    len += 1;
  }

  UART_PutString(&app.dev.uart, cstr, len);
}


typedef struct partition_entry_t {
  uint8_t bootable;
  uint8_t first_sector[3];
  uint8_t filesystem_type;
  uint8_t last_sector[3];
  uint32_t mbr_offset;
  uint32_t sector_count;
} partition_entry_t;

typedef struct boot_record_t {
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_count;
  uint8_t fat_count;
  uint16_t root_entry_count;
  uint32_t sector_count;
  uint16_t sectors_per_fat;
} boot_record_t;

typedef struct root_entry_t {
  uint8_t filename[8];
  uint8_t extension[3];
  uint8_t attributes;
  uint16_t cluster;
  uint32_t size;
} root_entry_t;

static sd_result_t sd_result;
static uint8_t sector[512];
static partition_entry_t partitions[4];
static boot_record_t boot;

bool MainState(AppState* self, event_t ev) {
  switch (ev) {
  // Application events
  case EVT_TIMER1:
    timerHandler(self);
    break;

  case EVT_BUTTON:
    buttonHandler(self);
    break;

  case EVT_UART:
    inputHandler(self);
    break;

  // Filesystem driver events (but the FS module doesn't exist yet)
  case EVT_SD_READY:
    switch (sd_result) {
    case SDR_OK:
      print("Success!\r\n");
      SD_GetSector(&self->dev.sd, 0ul, sector, mailbox(&self->eventBus, EVT_SD_MBR), &sd_result);
      break;

    case SDR_DEAD:
      print("SD Card Unresponsive\r\n");
      break;

    case SDR_COMPAT:
      print("Incompatible SD card\r\n");
      break;

    default:
      print("Unknown SD error.\r\n");
      break;
    }
    break;

  case EVT_SD_MBR:
    switch (sd_result) {
    case SDR_OK: {
      uint8_t id = 0;
      for (id = 0; id < 4; ++id) {
        uint16_t base = 0x1BE + 16*id;
        partitions[id].bootable = (bool)sector[base + 0x00];
        memcpy(&partitions[id].first_sector, &sector[base + 0x01], 3);
        partitions[id].filesystem_type = sector[base + 0x04];
        memcpy(&partitions[id].last_sector, &sector[base + 0x05], 3);
        partitions[id].mbr_offset =
            ((uint32_t)sector[base + 0x0B] << 24)
          | ((uint32_t)sector[base + 0x0A] << 16)
          | ((uint32_t)sector[base + 0x09] << 8)
          | ((uint32_t)sector[base + 0x08]);
        partitions[id].sector_count =
            ((uint32_t)sector[base + 0x0F] << 24)
          | ((uint32_t)sector[base + 0x0E] << 16)
          | ((uint32_t)sector[base + 0x0D] << 8)
          | ((uint32_t)sector[base + 0x0C]);
      }

      print("MBR!\r\n");
      if (partitions[0].filesystem_type != 0x06){
        print("FS not FAT16\r\n");
      } else {
        SD_GetSector(&self->dev.sd, partitions[0].mbr_offset, sector, mailbox(&self->eventBus, EVT_SD_PARTITION), &sd_result);
      }
      break;
    }

    default:
      print("Unknown SD error.\r\n");
      break;
    }
    break;

  case EVT_SD_PARTITION:
    switch (sd_result) {
    case SDR_OK:
      boot.bytes_per_sector =
          ((uint16_t)sector[0x0C] << 8)
        | ((uint16_t)sector[0x0B]);
      boot.sectors_per_cluster = sector[0x0D];
      boot.reserved_sector_count =
          ((uint16_t)sector[0x0F] << 8)
        | ((uint16_t)sector[0x0E]);
      boot.fat_count = sector[0x10];
      boot.root_entry_count =
          ((uint16_t)sector[0x12] << 8)
        | ((uint16_t)sector[0x11]);
      boot.sector_count =
          ((uint16_t)sector[0x14] << 8)
        | ((uint16_t)sector[0x13]);
      if (boot.sector_count == 0) {
        boot.sector_count =
          ((uint32_t)sector[0x23] << 24)
        | ((uint32_t)sector[0x22] << 16)
        | ((uint32_t)sector[0x21] << 8)
        | ((uint32_t)sector[0x20]);
      }
      boot.sectors_per_fat =
          ((uint16_t)sector[0x17] << 8)
        | ((uint16_t)sector[0x16]);

      print("Boot Record!\r\n");
      if (boot.bytes_per_sector != 512) {
        print("BPS not 512\r\n");
      } else {
        SD_GetSector(
          &self->dev.sd,
            partitions[0].mbr_offset
          + boot.reserved_sector_count
          + boot.sectors_per_fat * boot.fat_count,
          sector,
          mailbox(&self->eventBus, EVT_SD_ROOT),
          &sd_result
        );
      }
      break;

    default:
      print("Unknown SD error.\r\n");
      break;
    }
    break;

  case EVT_SD_ROOT: {
    switch (sd_result) {
    case SDR_OK:
      print("Root Entry!\r\n");

      uint32_t base = 0x40;

      root_entry_t entry;
      memcpy(&entry.filename, &sector[base + 0x00], 8);
      memcpy(&entry.extension, &sector[base + 0x08], 3);
      entry.attributes = sector[base + 0x0B];
      entry.cluster =
          ((uint16_t)sector[base + 0x1B] << 8)
        | ((uint16_t)sector[base + 0x1A]);
      entry.size =
          ((uint32_t)sector[base + 0x1F] << 24)
        | ((uint32_t)sector[base + 0x1E] << 16)
        | ((uint32_t)sector[base + 0x1D] << 8)
        | ((uint32_t)sector[base + 0x1C]);

      /*/
      sector[508] = 0xDE;
      sector[509] = 0xAD;
      sector[510] = 0xBE;
      sector[511] = 0xEF;
      uint32_t address =
          partitions[0].mbr_offset
        + boot.reserved_sector_count
        + boot.sectors_per_fat * boot.fat_count;
      SD_PutSector(&self->dev.sd, address, sector, &self->eventBus, EVT_DEBUG, &sd_response);
      //*/
      break;

    default:
      print("Unknown SD error.\r\n");
      break;
    }
    break;
  }

  case EVT_DEBUG: {
    print(" <><><>\r\n");
    break;
  }

  case EVT_IGNORE:
  default:
    break;
  }
}


int main() {
  EventBus_Init(&app.eventBus, NULL, 0);

  // Initialize SD state
  SD_Init(&app.dev.sd);

  // Initialize timer state
  Defer_Init(&app.dev.defer, 1000/*us*/);

  // Initialize button state
  app.dev.buttons.bus = &app.eventBus;
  app.dev.buttons.evt_CHANGE = EVT_BUTTON;

  // Initialize UART state
  UART_Init(&app.dev.uart, mailbox(&app.eventBus, EVT_UART));

  // Begin peripheral communications
  RawComm_Init(&app.dev);
  // LCD_Init(LCD_DISPLAY_NO_CURSOR, LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);
  SD_Reset(&app.dev.sd, mailbox(&app.eventBus, EVT_SD_READY), &sd_result);
  Defer_Set(&app.dev.defer, 500, &app.eventBus, EVT_TIMER1, NULL);

  while(1) {
    EventBus_Tick(&app.eventBus, (EventHandler)&MainState, &app);
  }
}
