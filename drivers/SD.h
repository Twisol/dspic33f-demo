#ifndef SD_H
#define SD_H

#include "../EventBus.h"
#include "../CircleBuffer.h"

#define SD_TIMEOUT_TICKS ((uint8_t)8)

typedef enum sd_result_t {
  SDR_OK = 0,
  SDR_INPROGRESS,
  SDR_DEAD,
  SDR_BUSY,
  SDR_COMPAT,
  SDR_CRC,
  SDR_FAULT,
} sd_result_t;

typedef enum sd_state_t {
  SDS_IDLE = 0,
  SDS_TRANSMIT,
  SDS_AWAIT_RECEIVE,
  SDS_RECEIVE,
} sd_state_t;

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
  EventBus* responseBus;  // Flow completion target
  event_t responseEvent;  // Flow completion event
  sd_result_t* responseResult;

  sd_flow_type_t flow_type;
  uint8_t flow_state;
  union {
    struct {} idle_flow;
    struct {} reset_flow;

    struct {
      uint32_t sector;
      uint8_t* buffer;
      uint16_t i;
    } read_single_flow;

    struct {
      uint32_t sector;
      const uint8_t* buffer;
      uint16_t i;
    } write_single_flow;
  };
} SdInterface;

// Exported API
bool SD_Init(SdInterface* sd);

bool SD_Reset(SdInterface* sd, EventBus* bus, event_t evt, sd_result_t* result);
bool SD_GetSector(SdInterface* sd, uint32_t sector, uint8_t buf[512], EventBus* bus, event_t evt, sd_result_t* result);
bool SD_PutSector(SdInterface* sd, uint32_t sector, const uint8_t buf[512], EventBus* bus, event_t evt, sd_result_t* result);

void SD_RecvRaw(SdInterface* sd, uint8_t data);

// Imported API
uint8_t RawComm_SD_Peek(SdInterface* sd);
void RawComm_SD_Poke(SdInterface* sd, uint8_t data);
bool RawComm_SD_CardDetected(SdInterface* sd);
bool RawComm_SD_WriteProtected(SdInterface* sd);

#endif /* SD_H */

