#include "SD.h"

#include "../CircleBuffer.h"

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
  case R1: return 1;
  case R1b: return 1;
  case R2: return 2;
  case R3: return 5;
  case R7: return 5;
  }
}


bool SD_Init(SdInterface* self) {
  CircleBuffer_Init(&self->rx);
  CircleBuffer_Init(&self->tx);
  self->idle = true;
  self->state = SDS_IDLE;
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
  self->target = bus;
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