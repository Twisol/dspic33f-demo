#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Imported API
void RawComm_Timer_Init(uint32_t period_us);

// Exported API
typedef void (*TimerEventHandler)();
int16_t Timer_Defer(uint16_t ticks, TimerEventHandler callback);
void Timer_EventHandler();

#endif  /* TIMER_H */

