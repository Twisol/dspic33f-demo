#ifndef SD_H
#define SD_H

#include "../EventBus.h"
#include "../CircleBuffer.h"

#define SD_TIMEOUT_TICKS ((uint8_t)8)

typedef enum sd_state_t {
  SDS_IDLE = 0,
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

typedef struct sd_command_t {
  unsigned : 2;
  unsigned index : 6; // Command index
  unsigned arg3 : 8;  // MSB of argument
  unsigned arg2 : 8;
  unsigned arg1 : 8;
  unsigned arg0 : 8;  // LSB of argument
} sd_command_t;

typedef enum sd_flow_type_t {
  SDF_IDLE = 0,
  SDF_RESET,
  SDF_READ_SINGLE,
  SDF_WRITE_SINGLE,
} sd_flow_type_t;


typedef struct SdInterface {
  // Driver state and orchestration
  sd_state_t state;
  CircleBuffer buffer;
  uint8_t rxBytesLeft;
  uint8_t txBytesLeft;
  uint8_t timeoutCount;
  uint8_t timeoutTicks;

  // Temporary data for use during action flows
  EventBus* responseBus; // Flow completion target
  event_t responseEvent;   // Flow completion event

  uint8_t flow_type;
  union {
    struct {
      uint8_t state;
    } idle_flow;

    struct {
      uint8_t state;
    } reset_flow;

    struct {
      uint8_t state;
      uint32_t sector;
      uint8_t* buffer;
      uint16_t i;
    } read_single_flow;

    struct {
      uint8_t state;
      uint32_t sector;
      const uint8_t* buffer;
      uint16_t i;
    } write_single_flow;
  };
} SdInterface;

// Exported API
bool SD_Init(SdInterface* sd);

bool SD_Reset(SdInterface* sd, EventBus* bus, event_t evt);
bool SD_GetSector(SdInterface* sd, uint32_t sector, uint8_t buf[512], EventBus* bus, event_t evt);
bool SD_PutSector(SdInterface* sd, uint32_t sector, const uint8_t buf[512], EventBus* bus, event_t evt);

void SD_RecvRaw(SdInterface* sd, uint8_t data);

// Imported API
uint8_t RawComm_SD_Peek(SdInterface* sd);
void RawComm_SD_Poke(SdInterface* sd, uint8_t data);
bool RawComm_SD_CardDetected(SdInterface* sd);
bool RawComm_SD_WriteProtected(SdInterface* sd);

#endif /* SD_H */

