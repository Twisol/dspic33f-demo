#include "Defer.h"

#include <stdint.h>

#include "EventBus.h"

volatile static uint16_t deferTable[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

static EventBus* g_eventBus = 0;
static uint16_t diffUnderflow = 0;
static uint16_t g_clockPeriod = 0;

void Defer_Init(uint16_t clockPeriod, EventBus* eventBus) {
  g_eventBus = eventBus;
  g_clockPeriod = clockPeriod;
}

bool Defer_Set(uint16_t ticks, uint8_t signal) {
  if (deferTable[signal] != 0) {
    return false;
  }

  deferTable[signal] = ticks;

  return true;
}

int16_t Defer_Clear(int16_t id) {
  deferTable[id] = 0;
}

void Defer_Tick(uint16_t timediff) {
  timediff += diffUnderflow;
  diffUnderflow = timediff % g_clockPeriod;

  uint16_t msPassed = timediff / g_clockPeriod;

  uint8_t signal = 0;
  for (signal = 0; signal < 32; ++signal) {
    if (deferTable[signal] == 0) {
      continue;
    }

    if (deferTable[signal] <= msPassed) {
      EventBus_Signal(g_eventBus, signal);

      deferTable[signal] = 0;
    } else {
      deferTable[signal] -= msPassed;
    }
  }
}