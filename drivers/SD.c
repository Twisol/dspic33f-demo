#include "SD.h"

#include "../CircleBuffer.h"

bool SD_Init(SdInterface* self) {
  CircleBuffer_Init(&self->rx);
  CircleBuffer_Init(&self->tx);
  self->overflow = false;
}

bool SD_CanSend(SdInterface* self) {
  return !CircleBuffer_IsFull(&self->tx);
}

bool SD_CanRecv(SdInterface* self) {
  return !CircleBuffer_IsEmpty(&self->tx);
}

uint16_t SD_Send(SdInterface* self, const uint8_t* buf, uint16_t len) {
  uint16_t bytesWritten = CircleBuffer_Write(&self->tx, buf, len);
  RawComm_SD_FlushTX(self); // Initiate the transfer
  return bytesWritten;
}

uint16_t SD_Recv(SdInterface* self, uint8_t* buf, uint16_t len) {
  uint16_t bytesRead = CircleBuffer_Read(&self->rx, buf, len);
  RawComm_SD_FlushRX(self); // Proactively read into the newly-available space
  return bytesRead;
}

void SD_ClearOverflow(SdInterface* self) {
  RawComm_SD_ClearOverflow(self);
}
