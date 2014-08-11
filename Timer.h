#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#include "EventBus.h"

void Timer_Init(uint16_t clockPeriod, EventBus* eventBus);
bool Timer_Defer(uint16_t ticks, uint8_t signal);
void Timer_Tick(uint16_t timediff);

#endif  /* TIMER_H */

