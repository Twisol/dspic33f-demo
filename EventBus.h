#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t Event;

struct EventBus;
typedef struct EventBus {
  struct EventBus* master;
  Event masterEvent;
  uint16_t tableSize;

  volatile bool signalTable[32];
} EventBus;

typedef bool (*EventHandler)(void* context, Event ev);
void EventBus_Signal(EventBus* self, Event ev);
void EventBus_Tick(EventBus* self, EventHandler handler, void* context);

#endif	/* EVENTQUEUE_H */

