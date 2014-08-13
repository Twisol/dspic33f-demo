#include "UART.h"

#include <stdbool.h>

#include "../CircleBuffer.h"

void UART_Init(UartBuffer* self) {
  CircleBuffer_Init(&self->data);
}

void UART_PutChar(UartBuffer* self, uint8_t ch) {
  while (!RawComm_UART_CanTransmit()) {
    continue;
  }

  RawComm_UART_PutChar(ch);
}

void UART_PutString(UartBuffer* self, const uint8_t* buf, uint8_t len) {
  uint8_t idx;
  for (idx = 0; idx < len; ++idx) {
    UART_PutChar(self, buf[idx]);
  }
}

bool UART_GetChar(UartBuffer* self, uint8_t* ch) {
  return CircleBuffer_Read(&self->data, ch, 1);
}

uint8_t UART_GetString(UartBuffer* self, uint8_t* dest, uint8_t len) {
  return CircleBuffer_Read(&self->data, dest, len);
}

uint8_t UART_GetCount(UartBuffer* self) {
  return CircleBuffer_Count(&self->data);
}


bool UART_Recv(UartBuffer* self, uint8_t ch) {
  return CircleBuffer_Write(&self->data, &ch, 1);
}