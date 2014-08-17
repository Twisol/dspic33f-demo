#include "RawComm.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <xc.h>            /* Register access */
#include <libpic30.h>      /* __delay32 and friends */

#include "EventBus.h"
#include "CircleBuffer.h"
#include "Defer.h"

#include "drivers/PushButtons.h"
#include "drivers/LCD.h"
#include "drivers/LED.h"
#include "drivers/UART.h"
#include "drivers/SD.h"

#define DEFAULT_PRIORITY 3
#define BAUD_RATE 9600
#define CLOCK_PERIOD 1000 /* microseconds */



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


void RawComm_PushButton_Init() {
  // Enable push buttons
  // Note that button 3 conflicts with LED 8, so we don't enable it.
  CNEN1bits.CN15IE = 0b1; // Button 1
  CNEN2bits.CN16IE = 0b1; // Button 2
  // CNEN2bits.CN23IE = 0b1; // Button 3
  CNEN2bits.CN19IE = 0b1; // Button 4

  // Enable interrupts
  IFS1bits.CNIF = 0b0;
  IPC4bits.CNIP = DEFAULT_PRIORITY;
  IEC1bits.CNIE = 0b1;
}

void __attribute__((interrupt,no_auto_psv)) _CNInterrupt() {
  if (IFS1bits.CNIF == 0)
    return;
  IFS1bits.CNIF = 0b0;

  RawComm* self = _InterruptGetRawComm();
  EventBus_Signal(self->buttons.bus, self->buttons.evt_CHANGE);
}


void RawComm_Timer_Init(uint32_t period_us) {
  // Set timing mode
  T2CONbits.TCS = 0b0;
  T2CONbits.TGATE = 0b0;
  T2CONbits.TCKPS = 0b00;

  TMR2 = 0;
  PR2 = (uint16_t)(period_us*FCY/1000000ULL);

  // Enable interrupts
  IFS0bits.T2IF = 0b0;
  IPC1bits.T2IP = DEFAULT_PRIORITY;
  IEC0bits.T2IE = 0b1;

  // Enable timer
  T2CONbits.TON = 0b1;
}

void __attribute__((interrupt,no_auto_psv)) _T2Interrupt() {
  if (IFS0bits.T2IF == 0b0) {
    return;
  }
  IFS0bits.T2IF = 0b0;

  RawComm* self = _InterruptGetRawComm();
  Defer_Tick(&self->defer, self->defer.clockPeriod);
}


void RawComm_UART_Init() {
  U2MODEbits.ABAUD = 0b0;
  U2MODEbits.BRGH = 0b0;
  U2BRG = (FCY/BAUD_RATE)/16 - 1;

  U2MODEbits.STSEL = 0b0;
  U2MODEbits.PDSEL = 0b00;
  U2STAbits.URXISEL = 0b0;

  IFS1bits.U2RXIF = 0b0;
  IPC7bits.U2RXIP = DEFAULT_PRIORITY;
  IEC1bits.U2RXIE = 0b1;

  U2MODEbits.UEN = 0b10;
  U2MODEbits.UARTEN = 0b1;
  U2STAbits.UTXEN = 0b1;
}

void RawComm_UART_PutChar(uint8_t ch) {
  U2TXREG = ch;
}

bool RawComm_UART_CanTransmit() {
  return !U2STAbits.UTXBF;
}

void __attribute__((interrupt,no_auto_psv)) _U2RXInterrupt() {
  if (IFS1bits.U2RXIF == 0b0) {
    return;
  }
  IFS1bits.U2RXIF = 0b0;

  uint8_t ch = U2RXREG;
  UART_RecvRaw(&_InterruptGetRawComm()->uart, ch);
}


// # SD/SPI Interface
//
// ## References
// - *SPI Module*: http://ww1.microchip.com/downloads/en/DeviceDoc/70005185a.pdf
// - *SD Daughter Board*: http://www.microchip.com/DevelopmentTools/ProductDetails.aspx?PartNO=AC164122
void RawComm_SD_Init() {
  TRISGbits.TRISG6 = 0; // SPI Clock Out
  TRISGbits.TRISG7 = 1; // SPI Data In
  TRISGbits.TRISG8 = 0; // SPI Data Out
  TRISGbits.TRISG0 = 1; // Card Insertion Detect pin
  TRISGbits.TRISG1 = 1; // Write-Protect Detect pin
  TRISBbits.TRISB9 = 0; // Card Select pin

  SPI2CON1bits.MSTEN = 1;
  SPI2CON1bits.CKE = 1;
  SPI2CON1bits.CKP = 0;
  SPI2CON1bits.SPRE = 0b010; //  6:1
  SPI2CON1bits.PPRE = 0b00;  // 64:1

  SPI2STATbits.SPIEN = 1;
  SPI2STATbits.SPIROV = 0;

  IFS2bits.SPI2IF = 0b0;
  IPC8bits.SPI2IP = DEFAULT_PRIORITY;
  IEC2bits.SPI2IE = 0b1;
}

uint8_t RawComm_SD_Peek(SdInterface* sd) {
  return SPI2BUF;
}

void RawComm_SD_Poke(SdInterface* sd, uint8_t data) {
  SPI2BUF = data;
}

bool RawComm_SD_CardDetected(SdInterface* sd) {
  return !PORTGbits.RG0;
}

bool RawComm_SD_WriteProtected(SdInterface* sd) {
  return !PORTGbits.RG1;
}

bool RawComm_SD_CardSelect(SdInterface* sd, bool enabled) {
  LATBbits.LATB9 = (enabled) ? 0 : 1; // Chip Select (On)
}

void __attribute__((interrupt,no_auto_psv)) _SPI2Interrupt() {
  if (IFS2bits.SPI2IF == 0b0) {
    return;
  }
  IFS2bits.SPI2IF = 0b0;

  uint8_t data = SPI2BUF;
  SD_RecvRaw(&_InterruptGetRawComm()->sd, data);
}


void RawComm_Init(RawComm* self) {
  RawComm_LED_Init();
  RawComm_LCD_Init();
  RawComm_UART_Init();
  RawComm_SD_Init();
  RawComm_PushButton_Init();
  RawComm_Timer_Init(self->defer.clockPeriod);
}