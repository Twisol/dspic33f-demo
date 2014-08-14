#ifndef SD_H
#define SD_H

#include "../EventBus.h"
#include "../CircleBuffer.h"

typedef struct SdInterface {
  // RawComm* env;
  EventBus* bus;
  uint8_t evt_RX;
  uint8_t evt_RX_OVERFLOW;

  CircleBuffer tx;
  CircleBuffer rx;
  bool overflow;
} SdInterface;

// Exported API
bool SD_Init(SdInterface* sd);
bool SD_CanSend(SdInterface* sd);
bool SD_CanRecv(SdInterface* sd);
uint16_t SD_Send(SdInterface* sd, const uint8_t* buf, uint16_t len);
uint16_t SD_Recv(SdInterface* sd, uint8_t* buf, uint16_t len);
bool SD_Overflow(SdInterface* sd);


// Imported API
void RawComm_SD_FlushTX(SdInterface* sd);
void RawComm_SD_FlushRX(SdInterface* sd);
void RawComm_SD_ClearOverflow(SdInterface* sd);

#endif /* SD_H */

