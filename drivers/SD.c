#include "SD.h"

#include "../CircleBuffer.h"
#include "../RawComm.h"  // Debugging

typedef enum sd_event_t {
  EVT_SD_RESET = 0,
  EVT_SD_CMD0,
  EVT_SD_CMD8,
} sd_event_t;


#define R1 SDR_R1
#define R1b SDR_R1b
#define R2 SDR_R2
#define R3 SDR_R3
#define R7 SDR_R7
sd_response_t cmdToResponseType[128] = {
  // Funamdental plane of commands  [0-63]
  R1, R1,  0,  0,   0,   0, R1,   0,
  R7, R1, R1,  0, R1b,  R2,  0,   0,
  R1, R1, R1,  0,   0,   0,  0,   0,
  R1, R1,  0, R1, R1b, R1b, R1,   0,
  R1, R1,  0,  0,   0,   0, R1b,  0,
  0,   0, R1,  0,   0,   0,  0,   0,
  0,   0,  0,  0,   0,   0,  0,  R1,
  R1,  0, R3, R1,   0,   0,  0,   0,

  // Application-specific commands  [64-127]
  0,   0,  0,  0,   0,   0,  0,   0,
  0,   0,  0,  0,   0,  R2,  0,   0,
  0,   0,  0,  0,   0,   0, R1,  R1,
  0,   0,  0,  0,   0,   0,  0,   0,
  0,   0,  0,  0,   0,   0,  0,   0,
  0,  R1, R1,  0,   0,   0,  0,   0,
  0,   0,  0, R1,   0,   0,  0,   0,
  0,   0,  0,  0,   0,   0,  0,   0,
};

uint8_t responseLength(sd_response_t type) {
  switch (type) {
  case R1:  return 1;
  case R1b: return 1;
  case R2:  return 2;
  case R3:  return 5;
  case R7:  return 5;
  }
}


// SD Specification: https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
bool SD_Init(SdInterface* self, EventBus* master, Event masterEvent) {
  EventBus_Init(&self->bus, master, masterEvent);

  CircleBuffer_Init(&self->rx);
  CircleBuffer_Init(&self->tx);
  self->idle = true;
  self->state = SDS_IDLE;
}

bool SD_Reset(SdInterface* sd) {
  SdCommand cmd = {SDC_CMD, 0, 0x00, 0x00, 0x00, 0x00};
  return SD_SendCommand(sd, cmd, &sd->bus, EVT_SD_CMD0);
}

uint8_t SD_GenCRC(SdCommand cmd) {
  switch (cmd.index) {
  case 0:
    return 0x4A;
  case 8:
    return 0x43;
  default:
    return 0xFF;
  }
}

bool SD_SendCommand(SdInterface* self, SdCommand cmd, EventBus* bus, Event evt) {
  if (self->state != SDS_IDLE) {
    return false;
  }

  uint8_t mem[] = {
    (cmd.index | 0x40) &~ 0x80,
    cmd.arg3, cmd.arg2, cmd.arg1, cmd.arg0,
    (SD_GenCRC(cmd) << 1) | 0x01
  };

  CircleBuffer_Write(&self->tx, mem, 6);
  self->responseBus = bus;
  self->responseEvent = evt;
  self->txBytesLeft = 6;
  self->rxBytesLeft = responseLength(cmdToResponseType[cmd.index]);

  if (self->idle) {
    self->idle = false;
    RawComm_SD_Poke(self, 0xFF);
  }
  return true;
}

bool SD_CardDetected(SdInterface* sd) {
  return RawComm_SD_CardDetected(sd);
}

bool SD_WriteProtected(SdInterface* sd) {
  return RawComm_SD_WriteProtected(sd);
}


void SD_EventHandler(SdInterface* sd, uint8_t signal) {
  RawComm* dev = _InterruptGetRawComm();

  switch (signal) {
  case EVT_SD_RESET:
    SD_Reset(sd);
    break;

  case EVT_SD_CMD0: {
    uint8_t response;
    CircleBuffer_Read(&sd->rx, &response, 1);

    if (response != 0x01) {
      SdCommand cmd = {SDC_CMD, 0, 0x00, 0x00, 0x00, 0x00};
      SD_SendCommand(sd, cmd, &sd->bus, EVT_SD_CMD0);
    } else {
      SdCommand cmd = {SDC_CMD, 8, 0x00, 0x00, 0x01, 0xAA};
      SD_SendCommand(sd, cmd, &sd->bus, EVT_SD_CMD8);
    }

    break;
  }

  case EVT_SD_CMD8: {
    uint8_t response[5];
    CircleBuffer_Read(&sd->rx, response, 5);

    if (response[0] == 0xFF) {
    // No response - retry
      UART_PutString(&dev->uart, "Response?\r\n", 11);
    }

    if (response[0] & 0x04) {
    // Illegal command - legacy card
      UART_PutString(&dev->uart, "Legacy?\r\n", 9);
      break;
    }

    if ((response[3] & 0x0F) == 0) {
    // Card does not support voltage level - don't retry
      UART_PutString(&dev->uart, "Voltage?\r\n", 10);
      break;
    }

    if (response[4] != 0xAA) {
    // Check-pattern does not match
      UART_PutString(&dev->uart, "Pattern?\r\n", 10);
      break;
    }

    // Modern card
    UART_PutString(&dev->uart, "Modern!\r\n", 9);

    sd->overflow = false;
    sd->idle = true;
    EventBus_Signal(sd->clientBus, sd->evt_READY);
    break;
  }

  default:
    break;
  }
}

void SD_ProcessEvents(SdInterface* sd) {
  EventBus_Tick(&sd->bus, (EventHandler)&SD_EventHandler, sd);
}


void SD_RecvRaw(SdInterface* sd, uint8_t data) {
  switch (sd->state) {
  case SDS_IDLE:
    if (data != 0xFF) {
    // busy?
      RawComm_SD_Poke(sd, 0xFF);
      break;
    } else if (sd->txBytesLeft == 0) {
      // We're waiting for more input, so signal that SPI2BUF may be pushed
      // from the main thread to resume.
      sd->idle = true;
      RawComm_SD_CardSelect(sd, false);
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
      RawComm_SD_Poke(sd, 0xFF);
    } else {
      uint8_t query;
      if (CircleBuffer_Read(&sd->tx, &query, 1)) {
        sd->txBytesLeft -= 1;
        sd->state = SDS_TRANSMIT;
        RawComm_SD_CardSelect(sd, true);
        RawComm_SD_Poke(sd, query);
      } else {
        sd->idle = true;
        RawComm_SD_CardSelect(sd, false);
      }
    }
    break;

  case SDS_AWAIT:
    if (data == 0xFF && sd->timeoutTicks > 0) {
    // awaiting response
      sd->timeoutTicks -= 1;
      RawComm_SD_Poke(sd, 0xFF);
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
      EventBus_Signal(sd->responseBus, sd->responseEvent);
    } else {
      if (CircleBuffer_Write(&sd->rx, &data, 1)) {
        sd->rxBytesLeft -= 1;
      } else {
        sd->overflow = true;
        sd->state = SDS_IDLE;
        EventBus_Signal(&sd->bus, EVT_SD_RESET);
      }
    }

    RawComm_SD_Poke(sd, 0xFF);
    break;

  default:
    EventBus_Signal(&sd->bus, EVT_SD_RESET);
    break;
  }
}