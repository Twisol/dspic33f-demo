#ifndef DEFER_H
#define DEFER_H

#include <stdint.h>

#include "EventBus.h"

typedef struct DeferTable {
  EventBus* bus;
  uint8_t size;
  uint16_t clockPeriod;
  volatile uint16_t diffUnderflow;
  volatile uint16_t countdowns[32];
} DeferTable;


void Defer_Init(DeferTable* self, uint16_t clockPeriod, EventBus* eventBus);
bool Defer_Set(DeferTable* self, uint16_t ticks, uint8_t signal);
void Defer_Tick(DeferTable* self, uint16_t timediff);

#endif  /* DEFER_H */

