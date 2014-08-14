#include <stddef.h>        /* NULL */
#include <stdint.h>        /* /u?int[8|16|32|64]_t */
#include <stdbool.h>       /* bool */

#include "EventBus.h"
#include "EventTypes.h"
#include "RawComm.h"
#include "Defer.h"

#include "drivers/LCD.h"
#include "drivers/LED.h"
#include "drivers/PushButtons.h"
#include "drivers/UART.h"

// Only supports base 2 through base 10 (i.e. not hexadecimal)
uint8_t* uitoab(uint16_t num, uint16_t base, uint8_t* buf, uint16_t len) {
  buf[len-1] = '\0';
  uint8_t* itr = buf + len-2;

  do {
    itr -= 1;

    if (itr < buf) {
      itr = 0;
      break;
    }

    int digit = num % base;
    num /= base;
    *itr = digit + '0';
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
    UART_PutString(&self->dev.uart, res, 17 - (res - buf));
    UART_PutString(&self->dev.uart, "\r\n", 2);
  }
}

void timerHandler(AppState* self) {
  // Display the current number
  self->count += 1;
  LED_Toggle(self->count);

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


bool MainState(AppState* self, Event ev) {
  switch (ev) {
  case EVT_TIMER1:
    timerHandler(self);
    break;
  case EVT_BUTTON:
    buttonHandler(self);
    break;
  case EVT_UART:
    inputHandler(self);
    break;
  case EVT_RAWCOMM:
    RawComm_ProcessEvents(&self->dev);
    break;
  }
}


int main() {
  EventBus_Init(&app.eventBus, NULL, 0);

  // Initialize timer state
  Defer_Init(&app.dev.defer, 1000/*us*/);

  // Initialize button state
  app.dev.buttons.bus = &app.eventBus;
  app.dev.buttons.evt_CHANGE = EVT_BUTTON;

  // Initialize LCD state
  LCD_Init(LCD_DISPLAY_NO_CURSOR, LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);

  // Initialize UART state
  app.dev.uart.bus = &app.eventBus;
  app.dev.uart.evt_RX = EVT_UART;
  UART_Init(&app.dev.uart);

  // Initialize hardware access layer and enable peripherals
  RawComm_Init(&app.dev, &app.eventBus, EVT_RAWCOMM);
  RawComm_Enable(&app.dev);

  Defer_Set(&app.dev.defer, 500, &app.eventBus, EVT_TIMER1, NULL);
  // Defer_Set(&app.deferTable, 1000, &app.eventBus, EVT_SD_INIT, NULL);

  while(1) {
    EventBus_Tick(&app.eventBus, (EventHandler)&MainState, &app);
  }
}
