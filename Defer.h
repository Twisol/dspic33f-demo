#ifndef DEFER_H
#define DEFER_H

#include <stdint.h>

#include "EventBus.h"

typedef struct DeferEntry {
  uint16_t countdown;
  EventBus* target;
  event_t ev;
} DeferEntry;

typedef struct DeferTable {
  uint16_t clockPeriod;
  uint16_t diffUnderflow;
  uint8_t size;

  volatile DeferEntry entries[32];
} DeferTable;


void Defer_Init(DeferTable* self, uint16_t clockPeriod);
bool Defer_Set(DeferTable* self, uint16_t ticks, EventBus* target, event_t ev, uint8_t* id);
void Defer_Tick(DeferTable* self, uint16_t timediff);

#endif  /* DEFER_H */

