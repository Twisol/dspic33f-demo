#include "Defer.h"

#include <stdint.h>
#include <string.h>

#include "EventBus.h"


void Defer_Init(DeferTable* self, uint16_t clockPeriod, EventBus* eventBus) {
  self->size = 32;
  self->bus = eventBus;
  self->clockPeriod = clockPeriod;
  memset((uint16_t*)self->countdowns, 0, self->size*sizeof(uint16_t));
}

bool Defer_Set(DeferTable* self, uint16_t ticks, uint8_t signal) {
  if (self->countdowns[signal] != 0) {
    return false;
  }

  self->countdowns[signal] = ticks;

  return true;
}

int16_t Defer_Clear(DeferTable* self, int16_t id) {
  self->countdowns[id] = 0;
}

void Defer_Tick(DeferTable* self, uint16_t timediff) {
  timediff += self->diffUnderflow;
  self->diffUnderflow = timediff % self->clockPeriod;

  uint16_t msPassed = timediff / self->clockPeriod;

  uint8_t signal = 0;
  for (signal = 0; signal < self->size; ++signal) {
    if (self->countdowns[signal] == 0) {
      continue;
    }

    if (self->countdowns[signal] <= msPassed) {
      EventBus_Signal(self->bus, signal);

      self->countdowns[signal] = 0;
    } else {
      self->countdowns[signal] -= msPassed;
    }
  }
}