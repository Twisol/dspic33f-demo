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


typedef enum dev_event_class_t {
  EVT_SD_RESET = 0,
  EVT_SD_CMD0,
  EVT_SD_CMD8,
} dev_event_class_t;


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

  RawComm* self = _InterruptGetRawComm();

  if (UART_GetCount(&self->uart) == 0) {
    EventBus_Signal(self->uart.bus, self->uart.evt_RX);
  }

  uint8_t ch = U2RXREG;
  UART_Recv(&self->uart, ch);
}


// # SD Card Driver
//
// ## References
// - *SD Specification*: https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
// - *SPI Module*: http://ww1.microchip.com/downloads/en/DeviceDoc/70005185a.pdf

void RawComm_SD_Init(RawComm* self) {
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

  EventBus_Signal(&self->bus, EVT_SD_RESET);
}

void __attribute__((interrupt,no_auto_psv)) _SPI2Interrupt() {
  if (IFS2bits.SPI2IF == 0b0) {
    return;
  }
  IFS2bits.SPI2IF = 0b0;

  RawComm* self = _InterruptGetRawComm();
  SdInterface* sd = &self->sd;

  uint8_t response = SPI2BUF;

  switch (sd->state) {
  case SDS_IDLE:
    if (response != 0xFF) {
    // busy?
      SPI2BUF = 0xFF;
      break;
    } else if (sd->txBytesLeft == 0) {
      // We're waiting for more input, so signal that SPI2BUF may be pushed
      // from the main thread to resume.
      sd->idle = true;
      LATBbits.LATB9 = 1; // Chip Select (Off)
      break;
    } else {
      sd->state = SDS_TRANSMIT;
      /* !!CASE FALL-THROUGH!! */
    }

  /* !!CASE FALL-THROUGH!! */
  case SDS_TRANSMIT:
    if (sd->txBytesLeft == 0) {
      sd->timeoutTicks = 8; // Number of 0xFF replies before timing out
      sd->state = SDS_AWAIT;
      SPI2BUF = 0xFF;
    } else {
      uint8_t query;
      if (CircleBuffer_Read(&sd->tx, &query, 1)) {
        LATBbits.LATB9 = 0; // Chip Select (On)
        sd->txBytesLeft -= 1;
        sd->state = SDS_TRANSMIT;
        SPI2BUF = query;
      } else {
        sd->idle = true;
        LATBbits.LATB9 = 1; // Chip Select (Off)
      }
    }
    break;

  case SDS_AWAIT:
    if (response == 0xFF && sd->timeoutTicks > 0) {
    // awaiting response
      sd->timeoutTicks -= 1;
      SPI2BUF = 0xFF;
      break;
    }

    // response received or timed out
    sd->state = SDS_RECEIVE;
    /* !!CASE FALL-THROUGH!! */

  /* !!CASE FALL-THROUGH!! */
  case SDS_RECEIVE:
    if (sd->rxBytesLeft == 0 || sd->timeoutTicks == 0) {
      uint8_t silence = 0xFF;
      while (sd->rxBytesLeft > 0) {
        CircleBuffer_Write(&sd->rx, &silence, 1);
        sd->rxBytesLeft -= 1;
      }

      sd->state = SDS_IDLE;
      EventBus_Signal(sd->target, sd->responseEvent);
    } else {
      if (CircleBuffer_Write(&sd->rx, &response, 1)) {
        sd->rxBytesLeft -= 1;
      } else {
        sd->overflow = true;
        sd->state = SDS_IDLE;
        EventBus_Signal(&self->bus, EVT_SD_RESET);
      }
    }

    SPI2BUF = 0xFF;
    break;

  default:
    EventBus_Signal(&self->bus, EVT_SD_RESET);
    break;
  }
}

bool RawComm_SD_Poke(SdInterface* sd, uint8_t data) {
  SPI2BUF = data;
}

bool RawComm_SD_CardDetected(SdInterface* sd) {
  return !PORTGbits.RG0;
}

bool RawComm_SD_WriteProtected(SdInterface* sd) {
  return !PORTGbits.RG1;
}

void RawComm_EventHandler(RawComm* self, uint8_t signal) {
  switch (signal) {
  case EVT_SD_RESET: {
    SdCommand cmd = {SDC_CMD, 0, 0x00, 0x00, 0x00, 0x00};
    SD_SendCommand(&self->sd, cmd, &self->bus, EVT_SD_CMD0);
    break;
  }

  case EVT_SD_CMD0: {
    uint8_t response;
    CircleBuffer_Read(&self->sd.rx, &response, 1);

    if (response != 0x01) {
      SdCommand cmd = {SDC_CMD, 0, 0x00, 0x00, 0x00, 0x00};
      SD_SendCommand(&self->sd, cmd, &self->bus, EVT_SD_CMD0);
    } else {
      SdCommand cmd = {SDC_CMD, 8, 0x00, 0x00, 0x01, 0xAA};
      SD_SendCommand(&self->sd, cmd, &self->bus, EVT_SD_CMD8);
    }

    break;
  }

  case EVT_SD_CMD8: {
    uint8_t response[5];
    CircleBuffer_Read(&self->sd.rx, response, 5);

    if (response[0] == 0xFF) {
    // No response - retry
      UART_PutString(&self->uart, "Response?\r\n", 11);
    }

    if (response[0] & 0x04) {
    // Illegal command - legacy card
      UART_PutString(&self->uart, "Legacy?\r\n", 9);
      break;
    }

    if ((response[3] & 0x0F) == 0) {
    // Card does not support voltage level - don't retry
      UART_PutString(&self->uart, "Voltage?\r\n", 10);
      break;
    }

    if (response[4] != 0xAA) {
    // Check-pattern does not match
      UART_PutString(&self->uart, "Pattern?\r\n", 10);
      break;
    }

    // Modern card
    UART_PutString(&self->uart, "Modern!\r\n", 9);

    self->sd.overflow = false;
    self->sd.idle = true;
    EventBus_Signal(self->sd.bus, self->sd.evt_READY);
    break;
  }

  default:
    // Unknown signal
    break;
  }
}

void RawComm_ProcessEvents(RawComm* self) {
  EventBus_Tick(&self->bus, (EventHandler)&RawComm_EventHandler, self);
}

void RawComm_Init(RawComm* self, EventBus* master, Event masterEvent) {
  EventBus_Init(&self->bus, master, masterEvent);
}

void RawComm_Enable(RawComm* self) {
  RawComm_LED_Init();
  RawComm_LCD_Init();
  RawComm_UART_Init();
  RawComm_SD_Init(self);
  RawComm_PushButton_Init();
  RawComm_Timer_Init(self->defer.clockPeriod);
}
