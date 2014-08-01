#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */

#include <xc.h>            /* Register access */
#include <libpic30.h>      /* __delay32 and friends */

#include "EventBus.h"
#include "EventTypes.h"
#include "drivers/LCD.h"
#include "drivers/LED.h"

#include "RawComm.h"


void PushButton_Init(PushButtonHandler handler) {
  RawComm_PushButton_Init(handler);
}

void buttonHandler() {
  LCD_PutString("Hello!");
}

static int count = 0;
void timerHandler() {
  // Display the current number
  LED_Toggle(count);
  count += 1;
}

int main() {
  RawComm_Init();

  LED_Init();
  
  PushButton_Init(&buttonHandler);

  LCD_Init();
  LCD_Display_Mode(LCD_DISPLAY_NO_CURSOR);
  LCD_Display_Clear();
  LCD_Draw_Mode(LCD_CURSOR_RIGHT, LCD_SHIFT_DISPLAY_OFF);

  EventBus_SetHook(EVT_TIMER, timerHandler);

  while (1) {
    EventBus_Signal(EVT_TIMER);
    EventBus_Tick();
    __delay_ms(500);
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
