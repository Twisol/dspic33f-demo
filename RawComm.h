#ifndef RAWCOMM_H
#define RAWCOMM_H

#include <stdint.h>

#include "EventBus.h"
#include "Defer.h"
#include "CircleBuffer.h"
#include "drivers/UART.h"
#include "drivers/PushButtons.h"
#include "drivers/SD.h"

typedef struct RawComm {
  DeferTable defer;
  UartBuffer uart;
  SdInterface sd;
  ButtonInterface buttons;
} RawComm;

void RawComm_Init(RawComm* self);

// Required to be implemented by the user
RawComm* _InterruptGetRawComm();

#endif	/* RAWCOMM_H */

