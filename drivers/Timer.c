#include "Timer.h"

#include <stdint.h>

static uint16_t deferTable[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};
static TimerEventHandler callbackTable[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

void Timer_Init(uint16_t period) {
  RawComm_Timer_Init(period);
}

int16_t Timer_Defer(uint16_t ticks, TimerEventHandler callback) {
  // Look for an unused slot
  uint8_t id;
  for (id = 0; id < 32; ++id) {
    if (deferTable[id] == 0) {
      break;
    }
  }

  if (id == 32) {
    return -1;
  }

  deferTable[id] = ticks;
  callbackTable[id] = callback;

  return id;
}

int16_t Timer_Release(int16_t id) {
  deferTable[id] = 0;
  callbackTable[id] = 0;
}

void Timer_EventHandler() {
  uint16_t id = 0;
  for (id = 0; id < 32; ++id) {
    if (deferTable[id] == 0) {
      continue;
    }

    deferTable[id] -= 1;
    if (deferTable[id] == 0) {
      TimerEventHandler callback = callbackTable[id];
      callbackTable[id] = 0;

      callback();
    }
  }
}