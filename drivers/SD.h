#ifndef SD_H
#define SD_H

#include "../EventBus.h"
#include "../CircleBuffer.h"

#define SD_TIMEOUT_TICKS ((uint8_t)8)

typedef enum sd_state_t {
  SDS_AWAIT_TRANSMIT = 0,
  SDS_TRANSMIT,
  SDS_AWAIT_RECEIVE,
  SDS_RECEIVE,
} sd_state_t;

typedef enum sd_response_t {
  SDR_INVALID = 0,
  SDR_R1,
  SDR_R1b,
  SDR_R2,
  SDR_R3,
  SDR_R7,
} sd_response_t;

struct SdCommand {
  unsigned : 2;
  unsigned index : 6; // Command index
  unsigned arg3 : 8;  // MSB of argument
  unsigned arg2 : 8;
  unsigned arg1 : 8;
  unsigned arg0 : 8;  // LSB of argument
};
struct SdCommand SdCommand(uint8_t index, uint8_t arg3, uint8_t arg2, uint8_t arg1, uint8_t arg0);

typedef enum sd_flow_type_t {
  SDF_IDLE = 0,
  SDF_RESET,
} sd_flow_type_t;


typedef struct SdInterface {
  // Driver state and orchestration
  sd_state_t state;
  CircleBuffer buffer;
  uint8_t rxBytesLeft;
  uint8_t txBytesLeft;
  uint8_t timeoutTicks;

  // Temporary data for use during action flows
  EventBus* responseBus; // Flow completion target
  event_t responseEvent;   // Flow completion event

  uint8_t flow_type;
  union {
    uint8_t idle_flow; // dummy variable
    struct {
      uint8_t state;
    } reset_flow;
    struct {} cmd_flow;
    struct {} acmd_flow;
  };
} SdInterface;

// Exported API
bool SD_Init(SdInterface* sd);
bool SD_Reset(SdInterface* sd, EventBus* bus, event_t evt);
bool SD_GetSector(SdInterface* sd, uint16_t sector, uint8_t buf[512], EventBus* bus, event_t evt);

void SD_RecvRaw(SdInterface* sd, uint8_t data);

// Imported API
uint8_t RawComm_SD_Peek(SdInterface* sd);
void RawComm_SD_Poke(SdInterface* sd, uint8_t data);
bool RawComm_SD_CardDetected(SdInterface* sd);
bool RawComm_SD_WriteProtected(SdInterface* sd);

#endif /* SD_H */

