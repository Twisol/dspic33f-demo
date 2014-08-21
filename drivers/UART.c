#include "UART.h"

#include <stdbool.h>

#include "../CircleBuffer.h"
#include "../EventBus.h"

void UART_Init(UartBuffer* self, mailbox_t onRX) {
  CircleBuffer_Init(&self->rx);
  CircleBuffer_Init(&self->tx);
  self->onRX = onRX;
}

void UART_PutChar(UartBuffer* self, uint8_t ch) {
  while (!RawComm_UART_CanTransmit()) {
    continue;
  }

  RawComm_UART_PutChar(ch);
}

void UART_PutString(UartBuffer* self, const uint8_t* buf, uint16_t len) {
  uint16_t idx;
  for (idx = 0; idx < len; ++idx) {
    UART_PutChar(self, buf[idx]);
  }
}

bool UART_GetChar(UartBuffer* self, uint8_t* ch) {
  return CircleBuffer_Read(&self->rx, ch, 1);
}

uint16_t UART_GetString(UartBuffer* self, uint8_t* dest, uint16_t len) {
  return CircleBuffer_Read(&self->rx, dest, len);
}

uint16_t UART_GetCount(UartBuffer* self) {
  return CircleBuffer_Count(&self->rx);
}


bool UART_RecvRaw(UartBuffer* self, uint8_t ch) {
  if (UART_GetCount(self) == 0) {
    Mailbox_Deliver(self->onRX);
  }

  return CircleBuffer_Write(&self->rx, &ch, 1);
}