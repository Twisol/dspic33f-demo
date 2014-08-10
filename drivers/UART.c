#include "UART.h"

static UARTHandler uartHandler = 0;
void UART_Init(UARTHandler handler) {
  uartHandler = handler;
  RawComm_UART_Init();
}

void UART_PutChar(uint8_t ch) {
  RawComm_UART_PutChar(ch);
}

void UART_PutString(uint8_t* str) {
  uint8_t* itr;
  for (itr = str; *itr != 0; ++itr) {
    RawComm_UART_PutChar(*itr);
  }
}

void UART_EventHandler(uint8_t ch) {
  uartHandler(ch);
}