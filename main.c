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


static int count = -1;
static EventBus eventBus;
static UartBuffer uart;

void buttonHandler(UartBuffer* uart) {
  // Display the current number
  uint8_t buf[17];
  uint8_t* res = uitoa(count, buf, 17);
  if (res != 0) {
    UART_PutString(uart, res);
    UART_PutString(uart, "\r\n");
  }
}

void timerHandler() {
  // Display the current number
  count += 1;
  LED_Toggle(count);

  Defer_Set(500, EVT_TIMER1);
}

void inputHandler(EventBus* bus, UartBuffer* uart) {
  uint8_t str[32];
  if (UART_GetString(uart, str, 32)) {
    UART_PutString(uart, str);
  }

  if (UART_GetCount(uart) > 0) {
    EventBus_Signal(bus, EVT_UART);
  }
}


bool HandleEvent(void* self, uint8_t signal) {
  switch (signal) {
  case EVT_TIMER1:
    timerHandler();
    break;
  case EVT_BUTTON:
    buttonHandler(&uart);
    break;
  case EVT_UART:
    inputHandler(&eventBus, &uart);
    break;
  }
}


int main() {
  EventBus_Init(&eventBus);
  RawComm_Init(&eventBus, &uart);

  Defer_Init(&eventBus, CLOCK_PERIOD);
  LCD_Init(LCD_DISPLAY_NO_CURSOR, LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);
  UART_Init(&uart);

  Defer_Set(500, EVT_TIMER1);

  while(1) {
    EventBus_Tick(&eventBus, &HandleEvent, NULL);
  }
}
