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


uint8_t* uitoa(uint16_t num, uint8_t* buf, size_t len) {
  buf[len-1] = '\0';
  uint8_t* itr = buf + len-2;

  do {
    itr -= 1;

    if (itr < buf) {
      itr = 0;
      break;
    }

    int digit = num % 10;
    num /= 10;
    *itr = digit + '0';
  } while (num > 0);

  return itr;
}


typedef struct AppState {
  EventBus eventBus;
  DeferTable deferTable;
  UartBuffer uart;

  int count;
} AppState;


static AppState app;

void buttonHandler(AppState* self) {
  // Display the current number
  uint8_t buf[17];
  uint8_t* res = uitoa(self->count, buf, 17);
  if (res != 0) {
    UART_PutString(&self->uart, res);
    UART_PutString(&self->uart, "\r\n");
  }
}

void timerHandler(AppState* self) {
  // Display the current number
  self->count += 1;
  LED_Toggle(self->count);

  Defer_Set(&self->deferTable, 500, EVT_TIMER1);
}

void inputHandler(AppState* self) {
  uint8_t str[32];
  if (UART_GetString(&self->uart, str, 32)) {
    UART_PutString(&self->uart, str);
  }

  if (UART_GetCount(&self->uart) > 0) {
    EventBus_Signal(&self->eventBus, EVT_UART);
  }
}


bool HandleEvent(AppState* self, uint8_t signal) {
  switch (signal) {
  case EVT_TIMER1:
    timerHandler(self);
    break;
  case EVT_BUTTON:
    buttonHandler(self);
    break;
  case EVT_UART:
    inputHandler(self);
    break;
  }
}


int main() {
  EventBus_Init(&app.eventBus);
  Defer_Init(&app.deferTable, CLOCK_PERIOD, &app.eventBus);

  RawComm_Init(&app.eventBus, &app.deferTable, &app.uart);
  LCD_Init(LCD_DISPLAY_NO_CURSOR, LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);
  UART_Init(&app.uart);

  Defer_Set(&app.deferTable, 500, EVT_TIMER1);

  while(1) {
    EventBus_Tick(&app.eventBus, (EventHandler)&HandleEvent, &app);
  }
}
