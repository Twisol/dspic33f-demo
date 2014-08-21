#include "SD.h"

#include "../CircleBuffer.h"


typedef struct sd_command_t {
  unsigned index : 6; // Command index
  unsigned arg3 : 8;  // MSB of argument
  unsigned arg2 : 8;
  unsigned arg1 : 8;
  unsigned arg0 : 8;  // LSB of argument
} sd_command_t;

inline
sd_command_t sd_command(uint8_t index, uint8_t arg3, uint8_t arg2, uint8_t arg1, uint8_t arg0) {
  sd_command_t cmd = {index, arg3, arg2, arg1, arg0};
  return cmd;
}

inline
sd_command_t cmd_go_idle_state() {
  return sd_command(0, 0x00, 0x00, 0x00, 0x00);
}

inline
sd_command_t cmd_send_if_cond(uint8_t pattern) {
  return sd_command(8, 0x00, 0x00, 0x01, pattern);
}

inline
sd_command_t cmd_app_cmd() {
  return sd_command(55, 0x00, 0x00, 0x00, 0x00);
}

inline
sd_command_t cmd_read_single_block(uint32_t address) {
  return sd_command(17,
    (address & 0xFF000000) >> 24,
    (address & 0x00FF0000) >> 16,
    (address & 0x0000FF00) >> 8,
    (address & 0x000000FF)
  );
}

inline
sd_command_t cmd_write_single_block(uint32_t address) {
  return sd_command(24,
    (address & 0xFF000000) >> 24,
    (address & 0x00FF0000) >> 16,
    (address & 0x0000FF00) >> 8,
    (address & 0x000000FF)
  );
}

inline
sd_command_t acmd_sd_send_op_cond() {
  return sd_command(41, 0x00, 0x00, 0x00, 0x00);
};


inline
uint8_t cmdCRC(sd_command_t cmd) {
  switch (cmd.index) {
  case 0:
    return 0x4A;
  case 8:
    return 0x43;
  default:
    return 0x55;
  }
}

inline
uint16_t sectorCRC(const uint8_t sector[512]) {
  return 0x55;
}


// SD Specification: https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
bool SD_Init(SdInterface* self) {
  CircleBuffer_Init(&self->buffer);
  self->state = SDS_TRANSMIT;
  self->timeoutCount = 0;

  self->flow_type = SDF_IDLE;
  self->flow_state = 0;
}


void SD_TransmitCmd(SdInterface* self, sd_command_t cmd, uint8_t recvLength, uint8_t timeout) {
  uint8_t mem[] = {
    (cmd.index | 0x40) &~ 0x80,
    cmd.arg3, cmd.arg2, cmd.arg1, cmd.arg0,
    (cmdCRC(cmd) << 1) | 0x01
  };

  CircleBuffer_Write(&self->buffer, mem, 6);
  self->txBytesLeft = 6;
  self->rxBytesLeft = recvLength;
  self->timeoutTicks = timeout;

  self->state = SDS_TRANSMIT;
  RawComm_SD_Poke(self, 0xFF);
}

void SD_Transmit(SdInterface* self, const uint8_t* buf, uint8_t length) {
  CircleBuffer_Write(&self->buffer, buf+1, length-1);
  self->txBytesLeft = length-1;
  self->rxBytesLeft = 0;
  self->timeoutTicks = 0;

  self->state = SDS_TRANSMIT;
  RawComm_SD_Poke(self, buf[0]);
}

void SD_Receive(SdInterface* self, uint8_t length, uint8_t timeout) {
  self->txBytesLeft = 0;
  self->rxBytesLeft = length;
  self->timeoutTicks = timeout;

  self->state = SDS_AWAIT_RECEIVE;
  RawComm_SD_Poke(self, 0xFF);
}

void SD_ReceiveNoWait(SdInterface* self, uint8_t length) {
  self->txBytesLeft = 0;
  self->rxBytesLeft = length;
  self->timeoutTicks = 0;

  self->state = SDS_RECEIVE; // dodge the await state
  RawComm_SD_Poke(self, 0xFF);
}

bool SD_CardDetected(SdInterface* sd) {
  return RawComm_SD_CardDetected(sd);
}

bool SD_WriteProtected(SdInterface* sd) {
  return RawComm_SD_WriteProtected(sd);
}


void SD_ResetFlow(SdInterface* sd) {
  switch (sd->flow_state) {
  case 0: {
    const uint8_t mem[10] = {
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };
    SD_Transmit(sd, mem, 10);
    sd->flow_state = 1;
    break;
  }

  case 1: {
    RawComm_SD_CardSelect(sd, true);

    SD_TransmitCmd(sd, cmd_go_idle_state(), 1, 8);
    sd->flow_state = 2;
    break;
  }

  case 2: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    if (response != 0x01) {
      SD_TransmitCmd(sd, cmd_go_idle_state(), 1, 8);
      sd->flow_state = 2;
    } else {
      SD_TransmitCmd(sd, cmd_send_if_cond(0xAA), 5, 8);
      sd->flow_state = 3;
    }
    break;
  }

  case 3: {
    uint8_t response[5];
    CircleBuffer_Read(&sd->buffer, response, 5);

    if (response[0] == 0xFF) {
    // No response
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_DEAD;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;
    }

    if (response[0] & 0x04) {
    // Illegal command - legacy card
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_COMPAT;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;
    }

    if ((response[3] & 0x0F) == 0) {
    // Card does not support voltage level - don't retry
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_COMPAT;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;
    }

    if (response[4] != 0xAA) {
    // Check-pattern does not match
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_COMPAT;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;
    }

    sd->flow_state = 4;
    SD_TransmitCmd(sd, cmd_app_cmd(), 1, 8);
    break;
  }

  case 4: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    sd->flow_state = 5;
    SD_TransmitCmd(sd, acmd_sd_send_op_cond(), 1, 8);
    break;
  }

  case 5: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    if (response == 0x00) {
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_OK;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
    } else {
      sd->flow_state = 4;
      SD_TransmitCmd(sd, cmd_app_cmd(), 1, 8);
    }
    break;
  }

  default:
    break;
  }
}

void SD_ReadSingleFlow(SdInterface* sd) {
  switch (sd->flow_state) {
  case 0: {
    RawComm_SD_CardSelect(sd, true);

    SD_TransmitCmd(sd, cmd_read_single_block(sd->read_single_flow.sector), 1, 8);
    sd->flow_state = 1;
    break;
  }

  case 1: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    // TODO: Handle errors

    SD_Receive(sd, 1, 128);
    sd->flow_state = 2;
    break;
  }

  case 2: {
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    // TODO: Handle errors

    SD_ReceiveNoWait(sd, 16);
    sd->flow_state = 3;
    break;
  }

  case 3: {
    uint8_t response[16];
    CircleBuffer_Read(&sd->buffer, response, 16);

    uint8_t i;
    for (i = 0; i < 16; ++i) {
      sd->read_single_flow.buffer[sd->read_single_flow.i + i] = response[i];
    }
    sd->read_single_flow.i += 16;

    if (sd->read_single_flow.i == 512) {
      SD_ReceiveNoWait(sd, 2);
      sd->flow_state = 4;
    } else {
      SD_ReceiveNoWait(sd, 16);
    }
    break;
  }

  case 4: {
    uint8_t crc[2];
    CircleBuffer_Read(&sd->buffer, crc, 2);

    RawComm_SD_CardSelect(sd, false);
    sd->flow_type = SDF_IDLE;
    sd->flow_state = 0;

    *sd->responseResult = SDR_OK;
    EventBus_Signal(sd->responseBus, sd->responseEvent);
    break;
  }

  default:
    break;
  }
}

void SD_WriteSingleFlow(SdInterface* sd) {
  switch (sd->flow_state) {
  case 0: { // Send command
    RawComm_SD_CardSelect(sd, true);

    SD_TransmitCmd(sd, cmd_write_single_block(sd->write_single_flow.sector), 1, 8);
    sd->flow_state = 1;
    break;
  }

  case 1: { // Send start-block
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    if (response != 0x00) {
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_BUSY;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
    } else {
      uint8_t start_block = 0xFE;
      SD_Transmit(sd, &start_block, 1);
      sd->flow_state = 2;
    }

    break;
  }

  case 2: { // Send block
    SD_Transmit(sd, sd->write_single_flow.buffer + sd->write_single_flow.i, 16);
    sd->write_single_flow.i += 16;

    if (sd->write_single_flow.i == 512) {
      sd->flow_state = 3;
    }
    break;
  }

  case 3: { // Send CDC
    uint16_t crc = sectorCRC(sd->write_single_flow.buffer);
    uint8_t bytes[2] = {(crc & 0xFF00) >> 8, (crc & 0x00FF)};
    SD_Transmit(sd, bytes, 2);
    sd->flow_state = 4;
    break;
  }

  case 4: { // Await response
    SD_Receive(sd, 1, 8);
    sd->flow_state = 5;
    break;
  }

  case 5: { // Receive response
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    switch ((response & 0x0E) >> 1) {
    case 0b010:
      SD_Receive(sd, 1, 8);
      sd->flow_state = 6;
      break;

    case 0b101:
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_CRC;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;

    case 0b110:
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_FAULT;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
      break;
    }

    break;
  }

  case 6: { // Wait for busy signal to complete
    uint8_t response;
    CircleBuffer_Read(&sd->buffer, &response, 1);

    if (response == 0x00) {
      SD_Receive(sd, 1, 8);
    } else {
      RawComm_SD_CardSelect(sd, false);
      sd->flow_type = SDF_IDLE;
      sd->flow_state = 0;

      *sd->responseResult = SDR_OK;
      EventBus_Signal(sd->responseBus, sd->responseEvent);
    }
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

  case SDF_READ_SINGLE:
    SD_ReadSingleFlow(sd);
    break;

  case SDF_WRITE_SINGLE:
    SD_WriteSingleFlow(sd);
    break;

  case SDF_IDLE:
  default:
    break;
  }
}


bool SD_Reset(SdInterface* sd, EventBus* bus, event_t evt, sd_result_t* result) {
  if (sd->flow_type != SDF_IDLE) {
    return false;
  }

  sd->flow_type = SDF_RESET;
  sd->flow_state = 0;

  sd->responseBus = bus;
  sd->responseEvent = evt;
  sd->responseResult = result;

  *result = SDR_INPROGRESS;
  SD_ResetFlow(sd);
  return true;
}

bool SD_GetSector(SdInterface* sd, uint32_t sector, uint8_t buf[512], EventBus* bus, event_t evt, sd_result_t* result) {
  if (sd->flow_type != SDF_IDLE) {
    return false;
  }

  sd->flow_type = SDF_READ_SINGLE;
  sd->flow_state = 0;

  sd->read_single_flow.sector = sector * 512ul; // address space translation
  sd->read_single_flow.buffer = buf;
  sd->read_single_flow.i = 0;

  sd->responseBus = bus;
  sd->responseEvent = evt;
  sd->responseResult = result;

  *result = SDR_INPROGRESS;
  SD_ReadSingleFlow(sd);
  return true;
}

bool SD_PutSector(SdInterface* sd, uint32_t sector, const uint8_t buf[512], EventBus* bus, event_t evt, sd_result_t* result) {
  if (sd->flow_type != SDF_IDLE) {
    return false;
  }

  sd->flow_type = SDF_WRITE_SINGLE;
  sd->flow_state = 0;

  sd->write_single_flow.sector = sector * 512ul; // address space translation
  sd->write_single_flow.buffer = buf;
  sd->write_single_flow.i = 0;

  sd->responseBus = bus;
  sd->responseEvent = evt;
  sd->responseResult = result;

  *result = SDR_INPROGRESS;
  SD_WriteSingleFlow(sd);
  return true;
}


void SD_RecvRaw(SdInterface* sd, uint8_t data) {
  switch (sd->state) {
  case SDS_TRANSMIT: {
    uint8_t query;

    if (sd->txBytesLeft != 0) {
      CircleBuffer_Read(&sd->buffer, &query, 1);

      sd->txBytesLeft -= 1;
      RawComm_SD_Poke(sd, query);
      break;
    } else {
      sd->state = SDS_AWAIT_RECEIVE;
      /* !!CASE FALLTHROUGH!! */
    }
  }

  /* !!CASE FALLTHROUGH!! */
  case SDS_AWAIT_RECEIVE:
    if (data == 0xFF && sd->rxBytesLeft != 0 && sd->timeoutCount < sd->timeoutTicks) {
    // awaiting response
      sd->timeoutCount += 1;
      RawComm_SD_Poke(sd, 0xFF);
      break;
    } else if (sd->timeoutCount >= sd->timeoutTicks) {
    // timed out
      uint8_t silence = 0xFF;
      while (sd->txBytesLeft > 0) {
        CircleBuffer_Read(&sd->buffer, &silence, 1);
        sd->txBytesLeft -= 1;
      }
      while (sd->rxBytesLeft > 0) {
        CircleBuffer_Write(&sd->buffer, &silence, 1);
        sd->rxBytesLeft -= 1;
      }

      sd->state = SDS_IDLE;
      sd->timeoutCount = 0;
      SD_DoNext(sd);
      break;
    } else {
    // response received
      sd->state = SDS_RECEIVE;
      /* !!CASE FALLTHROUGH!! */
    }

  /* !!CASE FALLTHROUGH!! */
  case SDS_RECEIVE:
    if (sd->rxBytesLeft > 0) {
      CircleBuffer_Write(&sd->buffer, &data, 1);
      sd->rxBytesLeft -= 1;
    }

    if (sd->rxBytesLeft == 0) {
      sd->state = SDS_IDLE;
      sd->timeoutCount = 0;
      SD_DoNext(sd);
    } else {
      RawComm_SD_Poke(sd, 0xFF);
    }

    break;

  case SDS_IDLE:
  default:
    break;
  }
}
