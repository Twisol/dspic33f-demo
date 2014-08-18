#include "SD.h"

#include "../CircleBuffer.h"
#include "../RawComm.h"  // Debugging

sd_command_t sd_command(uint8_t index, uint8_t arg3, uint8_t arg2, uint8_t arg1, uint8_t arg0) {
  sd_command_t cmd = {index, arg3, arg2, arg1, arg0};
  return cmd;
}


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

uint8_t genCRC(sd_command_t cmd) {
  switch (cmd.index) {
  case 0:
    return 0x4A;
  case 8:
    return 0x43;
  default:
    return 0xFF;
  }
}


// SD Specification: https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
bool SD_Init(SdInterface* self) {
  CircleBuffer_Init(&self->buffer);
  self->state = SDS_AWAIT_TRANSMIT;
  self->flow_type = SDF_IDLE;
}

void SD_TransmitCmd(SdInterface* self, sd_command_t cmd, uint8_t recvLength) {
  uint8_t mem[] = {
    (cmd.index | 0x40) &~ 0x80,
    cmd.arg3, cmd.arg2, cmd.arg1, cmd.arg0,
    (genCRC(cmd) << 1) | 0x01
  };

  CircleBuffer_Write(&self->buffer, mem, 6);
  self->txBytesLeft = 6;
  self->rxBytesLeft = recvLength;

  RawComm_SD_CardSelect(self, true);
  RawComm_SD_Poke(self, 0xFF);
}

bool SD_GetSector(SdInterface* self, uint16_t sector, uint8_t buf[512], EventBus* bus, event_t evt) {
  if (self->flow_type != SDF_IDLE) {
    return false;
  }

  uint16_t i;
  for (i = 0; i < 512; ++i) {
    buf[i] = 'a' + (i % 26);
  }

  EventBus_Signal(bus, evt);
  return true;
}

bool SD_CardDetected(SdInterface* sd) {
  return RawComm_SD_CardDetected(sd);
}

bool SD_WriteProtected(SdInterface* sd) {
  return RawComm_SD_WriteProtected(sd);
}


void SD_ResetFlow(SdInterface* sd) {
  RawComm* dev = _InterruptGetRawComm();

  uint8_t* state = &sd->reset_flow.state;

  switch (sd->reset_flow.state) {
  case 0:
    RawComm_SD_CardSelect(sd, true);

    SD_TransmitCmd(sd, sd_command(0, 0x00, 0x00, 0x00, 0x00), 1);
    *state = 1;
    break;

  case 1: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    if (response != 0x01) {
      SD_TransmitCmd(sd, sd_command(0, 0x00, 0x00, 0x00, 0x00), 1);
      *state = 1;
    } else {
      SD_TransmitCmd(sd, sd_command(8, 0x00, 0x00, 0x01, 0xAA), 5);
      *state = 3;
    }
    break;
  }

  case 2: {
    uint8_t response[5];
    CircleBuffer_Read(&sd->buffer, response, 5);
    RawComm_SD_CardSelect(sd, false);

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
    EventBus_Signal(sd->responseBus, sd->responseEvent);

    sd->flow_type = SDF_IDLE;
    sd->idle_flow = 0;
    *state = 0;
    break;
  }

  default:
    break;
  }
}

void SD_DoNext(SdInterface* sd) {
  switch (sd->flow_type) {
  case SDF_RESET:
    SD_ResetFlow(sd);
    break;

  case SDF_IDLE:
  default:
    break;
  }
}

bool SD_Reset(SdInterface* sd, EventBus* bus, event_t evt) {
  if (sd->flow_type != SDF_IDLE) {
    return false;
  }

  sd->flow_type = SDF_RESET;
  sd->reset_flow.state = 0;

  sd->responseBus = bus;
  sd->responseEvent = evt;
  SD_ResetFlow(sd);
}


void SD_RecvRaw(SdInterface* sd, uint8_t data) {
  switch (sd->state) {
  case SDS_AWAIT_TRANSMIT:
    if (data != 0xFF && sd->txBytesLeft != 0 && sd->timeoutTicks != 0) {
    // awaiting go-ahead
      sd->timeoutTicks -= 1;
      RawComm_SD_Poke(sd, 0xFF);
      break;
    } else {
    // ready to send
      sd->state = SDS_TRANSMIT;
      /* !!CASE FALLTHROUGH!! */
    }

  /* !!CASE FALLTHROUGH!! */
  case SDS_TRANSMIT: {
    uint8_t query;

    if (sd->txBytesLeft != 0 && sd->timeoutTicks != 0) {
      CircleBuffer_Read(&sd->buffer, &query, 1);

      sd->txBytesLeft -= 1;
      RawComm_SD_Poke(sd, query);
      break;
    } else {
      uint8_t silence = 0xFF;
      while (sd->txBytesLeft > 0) {
        CircleBuffer_Read(&sd->buffer, &silence, 1);
        sd->txBytesLeft -= 1;
      }

      sd->state = SDS_AWAIT_RECEIVE;
      sd->timeoutTicks = SD_TIMEOUT_TICKS;
      /* !!CASE FALLTHROUGH!! */
    }
  }

  /* !!CASE FALLTHROUGH!! */
  case SDS_AWAIT_RECEIVE:
    if (data == 0xFF && sd->rxBytesLeft != 0 && sd->timeoutTicks != 0) {
    // awaiting response
      sd->timeoutTicks -= 1;
      RawComm_SD_Poke(sd, 0xFF);
      break;
    } else {
    // response received or timed out
      /* !!CASE FALLTHROUGH!! */
    }

  /* !!CASE FALLTHROUGH!! */
  case SDS_RECEIVE:
    if (sd->rxBytesLeft != 0 && sd->timeoutTicks != 0) {
      CircleBuffer_Write(&sd->buffer, &data, 1);
      sd->rxBytesLeft -= 1;
      RawComm_SD_Poke(sd, 0xFF);
    } else {
      uint8_t silence = 0xFF;
      while (sd->rxBytesLeft > 0) {
        CircleBuffer_Write(&sd->buffer, &silence, 1);
        sd->rxBytesLeft -= 1;
      }

      sd->state = SDS_AWAIT_TRANSMIT;
      sd->timeoutTicks = SD_TIMEOUT_TICKS;
      SD_DoNext(sd);
    }
    break;

  default:
    break;
  }
}
