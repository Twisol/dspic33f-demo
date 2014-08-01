#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>

typedef void (*EventHandler)();

void EventBus_Signal(uint8_t type);
void EventBus_SetHook(uint8_t type, EventHandler handler);
void EventBus_Tick();

#endif	/* EVENTQUEUE_H */

