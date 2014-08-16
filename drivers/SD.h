#ifndef SD_H
#define SD_H

#include "../EventBus.h"
#include "../CircleBuffer.h"

typedef enum sd_state_t {
  SDS_IDLE = 0,
  SDS_TRANSMIT,
  SDS_AWAIT,
  SDS_RECEIVE,
  SDS_OVERFLOW,
} sd_state_t;

typedef enum sd_response_t {
  SDR_INVALID = 0,
  SDR_R1,
  SDR_R1b,
  SDR_R2,
  SDR_R3,
  SDR_R7,
} sd_response_t;

typedef enum sd_cmd_type_t {
  SDC_CMD = 0,
  SDC_ACMD = 1,
} sd_cmd_type_t;

typedef struct SdCommand {
  unsigned : 1;
  unsigned app : 1;   // ACMD flag: 1->ACMD, 0->CMD
  unsigned index : 6; // Command index
  unsigned arg3 : 8;  // MSB of argument
  unsigned arg2 : 8;
  unsigned arg1 : 8;
  unsigned arg0 : 8;  // LSB of argument
} SdCommand;


typedef struct SdInterface {
// public:
  EventBus* bus;
  Event evt_READY;
  Event evt_RX_OVERFLOW;

// private:
  CircleBuffer tx;
  CircleBuffer rx;
  sd_state_t state;
  bool idle;
  bool overflow;

  uint8_t rxBytesLeft;
  uint8_t txBytesLeft;
  uint8_t timeoutTicks;
  EventBus* target;
  Event responseEvent;
} SdInterface;

// Exported API
bool SD_Init(SdInterface* sd);
bool SD_SendCommand(SdInterface* sd, SdCommand cmd, EventBus* bus, Event evt);

// Imported API
bool RawComm_SD_Poke(SdInterface* sd, uint8_t data);
bool RawComm_SD_CardDetected(SdInterface* sd);
bool RawComm_SD_WriteProtected(SdInterface* sd);

#endif /* SD_H */

