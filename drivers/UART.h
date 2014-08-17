#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

#include "../CircleBuffer.h"
#include "../EventBus.h"

typedef struct UartBuffer {
  EventBus* bus;
  Event evt_RX;

  CircleBuffer rx;
  CircleBuffer tx;
} UartBuffer;

void RawComm_UART_PutChar(uint8_t ch);
bool RawComm_UART_CanTransmit();

// Exported API
void UART_Init(UartBuffer* self);
void UART_PutChar(UartBuffer* self, uint8_t ch);
void UART_PutString(UartBuffer* self, const uint8_t* buf, uint16_t len);

bool UART_GetChar(UartBuffer* self, uint8_t* ch);
uint16_t UART_GetString(UartBuffer* self, uint8_t* dest, uint16_t len);

bool UART_RecvRaw(UartBuffer* self, uint8_t ch);
uint16_t UART_Count(UartBuffer* self);

#endif	/* UART_H */

