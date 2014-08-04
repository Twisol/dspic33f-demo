#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include <libpic30.h>      /* __delay32 and friends */

#include "EventBus.h"
#include "EventTypes.h"
#include "RawComm.h"

#include "drivers/LCD.h"
#include "drivers/LED.h"
#include "drivers/PushButtons.h"
#include "drivers/Timer.h"


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

int main() {
  EventBus_Init();

  LED_Init();

  LCD_Init();
  LCD_Display_Mode(LCD_DISPLAY_NO_CURSOR);
  LCD_Display_Clear();
  LCD_Draw_Mode(LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);

  PushButton_Init(&buttonHandler);

  Timer_Init(1000);
  Timer_Defer(500, &timerHandler);

  while(1) {
    EventBus_Tick();
  }

  /*while (1) {
    // Toggle LEDs
    LATA = (LATA &~ 0x55) | 0xAA;
    __delay_ms(500);

    LCD_Display_Clear();

    LATA = (LATA &~ 0xAA) | 0x55;
    __delay_ms(500);
  };*/

  /*
  INTCON1bits.NSTDIS = 0b1;

  // Configure UART interface 1
  // Disable interrupts
  IEC0bits.U1TXIE = 0b0;
  IEC0bits.U1RXIE = 0b0;
  IEC4bits.U1EIE = 0b0;

  IPC3bits.U1TXIP = 0b0000;
  IPC2bits.U1RXIP = 0b0000;
  IPC16bits.U1EIP = 0b0000;

  U1MODEbits.UARTEN = 0b1;
  U1MODEbits.USIDL = 0b0;
  U1MODEbits.IREN = 0b1;
  U1MODEbits.RTSMD = 0b0;
  U1MODEbits.UEN = 0b00;
  U1MODEbits.WAKE = 0b0;
  U1MODEbits.LPBACK = 0b0;
  U1MODEbits.ABAUD = 0b0;
  U1MODEbits.PDSEL = 0b00;
  U1MODEbits.STSEL = 0b0;

  U1BRG = ((Fcyc/9600)/16)-1; //38400 baud

  U1STAbits.UTXISEL0 = 0b0;
  U1STAbits.UTXISEL1 = 0b0;
  U1STAbits.UTXINV = 0b0;
  U1STAbits.UTXBRK = 0b0;
  U1STAbits.UTXEN = 0b0;
  U1STAbits.URXISEL = 0b00;
  U1STAbits.ADDEN = 0b0;

  U1TXREG = 'y' & 0x00FF;
  while (!U1STAbits.TRMT)
    continue;

  while(1)
  {
    while (!U1STAbits.URXDA)
      continue;
    char ch = U1RXREG & 0x00FF;

    U1TXREG = ch & 0x00FF;
    while (!U1STAbits.TRMT)
      continue;
  }
  */
}
