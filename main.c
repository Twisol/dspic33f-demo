#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include "EventBus.h"
#include "EventTypes.h"
#include "RawComm.h"

#include "drivers/LCD.h"
#include "drivers/LED.h"
#include "drivers/PushButtons.h"
#include "drivers/Timer.h"
#include "drivers/UART.h"


static int count = -1;

void buttonHandler() {
  // Display the current number (with digits reversed...)
  LCD_Display_Clear();

  int tmp = count;
  if (tmp == 0) {
    LCD_PutChar('0');
    return;
  }

  while (tmp != 0) {
    int digit = tmp % 10;
    tmp /= 10;
    LCD_PutChar(digit + '0');
  }
}

void timerHandler() {
  // Display the current number
  count += 1;
  LED_Toggle(count);

  Timer_Defer(500, &timerHandler);
}

void inputHandler(uint8_t ch) {
  UART_PutChar(ch);
}

int main() {
  EventBus eventBus;
  EventBus_Init(&eventBus);

  RawComm_Init(&eventBus, 1000/*us*/);

  LCD_Init();
  LCD_Display_Mode(LCD_DISPLAY_NO_CURSOR);
  LCD_Display_Clear();
  LCD_Draw_Mode(LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);

  UART_Init(&inputHandler);
  PushButton_Init(&buttonHandler);

  Timer_Defer(500, &timerHandler);

  while(1) {
    EventBus_Tick(&eventBus);
  }
}
