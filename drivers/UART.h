#ifndef UART_H
#define UART_H

#include <stdint.h>

// Imported API
void RawComm_UART_Init();
void RawComm_UART_PutChar(uint8_t ch);

// Exported API
typedef void (*UARTHandler)(uint8_t ch);
void UART_Init(UARTHandler handler);
void UART_EventHandler(uint8_t ch);

#endif	/* UART_H */

