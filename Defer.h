#ifndef DEFER_H
#define DEFER_H

#include <stdint.h>

#include "EventBus.h"

void Defer_Init(uint16_t clockPeriod, EventBus* eventBus);
bool Defer_Set(uint16_t ticks, uint8_t signal);
void Defer_Tick(uint16_t timediff);

#endif  /* DEFER_H */

