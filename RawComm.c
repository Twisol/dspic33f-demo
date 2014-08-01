#include "RawComm.h"

#include <stdbool.h>
#include <stdint.h>

#include <xc.h>            /* Register access */
#include <libpic30.h>      /* __delay32 and friends */

#include "EventBus.h"
#include "EventTypes.h"


enum {
  HIGH_PRIORITY = 7,
  MID_PRIORITY = 4,
  LOW_PRIORITY = 1,
};

void RawComm_LCD_Init() {
  __delay_ms(30);
  TRISE = TRISE &~ 0x00FF; // Enable LCD output
  TRISD = TRISD &~ 0x0030;
  TRISB = TRISB &~ 0x8000;
}

// LCD controller: http://ww1.microchip.com/downloads/en/DeviceDoc/NT7603_V2.3.pdf
void RawComm_LCD_Send(bool is_data, bool do_read, uint64_t delay, uint16_t payload) {
  LATBbits.LATB15 = is_data;  // RS
  LATDbits.LATD5 = do_read;   // R/W
  LATE = payload;             // DB

  LATDbits.LATD4 = 0b1;       // E
  __delay_us(delay);
  LATDbits.LATD4 = 0b0;
}


void RawComm_LED_Init() {
  TRISA = TRISA &~ 0x00FF; // Enable LED output
}

void RawComm_LED_Toggle(uint8_t bitvector) {
  LATA = (LATA &~ 0x00FF) | bitvector;
}


static PushButtonHandler buttonHandler = 0;
void RawComm_PushButton_EventHandler() {
  IPC4bits.CNIP = LOW_PRIORITY;
  if (buttonHandler) {
    buttonHandler();
  }
  IPC4bits.CNIP = MID_PRIORITY;
}

void RawComm_PushButton_Init(PushButtonHandler hook) {
  // Note that button 3 conflicts with LED 8, so we don't enable it.
  TRISD = TRISD | 0x1060;

  CNEN1bits.CN15IE = 0b1; // Button 1
  CNEN2bits.CN16IE = 0b1; // Button 2
  CNEN2bits.CN19IE = 0b1; // Button 4

  IFS1bits.CNIF = 0b0;
  IPC4bits.CNIP = MID_PRIORITY;
  IEC1bits.CNIE = 0b1;

  // Associate hook with PushButton events
  buttonHandler = hook;
  EventBus_SetHook(EVT_BUTTON, &RawComm_PushButton_EventHandler);
}

void __attribute__((interrupt,no_auto_psv)) _CNInterrupt() {
  IFS1bits.CNIF = 0;

  // Push an event to the event queue
  EventBus_Signal(EVT_BUTTON);
}

void RawComm_Init() {
  SRbits.IPL = MID_PRIORITY;
}