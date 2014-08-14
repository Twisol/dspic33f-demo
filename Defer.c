#include "Defer.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "EventBus.h"


void Defer_Init(DeferTable* self, uint16_t clockPeriod) {
  self->size = 32;
  self->clockPeriod = clockPeriod;
  memset((DeferEntry*)self->entries, 0, self->size*sizeof(DeferEntry));
}

bool Defer_Set(DeferTable* self, uint16_t ticks, EventBus* target, Event ev, uint8_t* id) {
  uint8_t idx;
  for (idx = 0; idx < self->size; ++idx) {
    if (self->entries[idx].countdown != 0) {
      continue;
    }

    self->entries[idx].countdown = ticks;
    self->entries[idx].target = target;
    self->entries[idx].ev = ev;

    if (id != NULL) {
      *id = idx;
    }
    return true;
  }

  return false;
}

int16_t Defer_Clear(DeferTable* self, int16_t id) {
  // A non-zero countdown shall only be modified by the main thread.
  // A NULL target for an active timer signals that the timer has been cancelled.
  self->entries[id].target = NULL;
}

void Defer_Tick(DeferTable* self, uint16_t timediff) {
  timediff += self->diffUnderflow;
  self->diffUnderflow = timediff % self->clockPeriod;

  uint16_t msPassed = timediff / self->clockPeriod;

  uint8_t id = 0;
  for (id = 0; id < self->size; ++id) {
    if (self->entries[id].countdown == 0) {
      continue;
    }

    uint8_t ev = self->entries[id].ev;
    EventBus* target = self->entries[id].target;
    uint16_t countdown = self->entries[id].countdown;

    if (target != NULL) {
      if (countdown <= msPassed) {
        EventBus_Signal(target, ev);
      } else {
        self->entries[id].countdown = countdown - msPassed;
      }
    }

    if (countdown <= msPassed || target == NULL) {
      self->entries[id].ev = 0;
      self->entries[id].target = NULL;
      self->entries[id].countdown = 0;
    }
  }
}