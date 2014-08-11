#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Exported API
typedef void (*TimerEventHandler)();
int16_t Timer_Defer(uint16_t ticks, TimerEventHandler callback);
void Timer_EventHandler();

#endif  /* TIMER_H */

