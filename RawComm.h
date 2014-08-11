#ifndef RAWCOMM_H
#define RAWCOMM_H

#include <stdint.h>

#include "EventBus.h"
#include "drivers/UART.h"

void RawComm_Init(EventBus* eventBus, UartBuffer* uart);

#define CLOCK_PERIOD 1000 /* microseconds */

#endif	/* RAWCOMM_H */

