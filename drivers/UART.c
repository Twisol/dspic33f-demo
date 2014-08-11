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

void UART_PutString(UartBuffer* self, uint8_t* str) {
  uint8_t* itr = str;
  while (*itr != 0) {
    UART_PutChar(self, *itr);
    itr += 1;
  }
}

bool UART_GetChar(UartBuffer* self, uint8_t* ch) {
  return CircleBuffer_Pop(&self->data, ch);
}

bool UART_GetString(UartBuffer* self, uint8_t* dest, uint8_t len) {
  uint8_t idx = 0;
  while (idx < len-1) {
    if (!UART_GetChar(self, dest+idx)) {
      break;
    }

    idx += 1;
  }

  dest[idx] = '\0';
  return (idx != 0);
}

uint8_t UART_GetCount(UartBuffer* self) {
  return CircleBuffer_Count(&self->data);
}


bool UART_Recv(UartBuffer* self, uint8_t ch) {
  return CircleBuffer_Push(&self->data, ch);
}