#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>

#include "../CircleBuffer.h"

typedef struct UartBuffer {
  CircleBuffer data;
} UartBuffer;

// Imported API
void RawComm_UART_PutChar(uint8_t ch);
bool RawComm_UART_CanTransmit();

// Exported API
void UART_Init(UartBuffer* self);
void UART_PutChar(UartBuffer* self, uint8_t ch);
void UART_PutString(UartBuffer* self, const uint8_t* buf, uint8_t len);

bool UART_GetChar(UartBuffer* self, uint8_t* ch);
uint8_t UART_GetString(UartBuffer* self, uint8_t* dest, uint8_t len);

bool UART_Recv(UartBuffer* self, uint8_t ch);
uint8_t UART_GetCount(UartBuffer* self);

#endif	/* UART_H */

