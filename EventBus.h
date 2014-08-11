#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct EventBus {
  volatile bool signalTable[32];
} EventBus;

typedef bool (*EventHandler)(void* context, uint8_t signal);
void EventBus_Signal(EventBus* self, uint8_t signal);
void EventBus_Tick(EventBus* self, EventHandler handler, void* context);

#endif	/* EVENTQUEUE_H */

