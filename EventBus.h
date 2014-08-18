#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t event_t;

typedef struct EventBus EventBus;
struct EventBus {
  EventBus* master;
  event_t masterEvent;
  uint16_t tableSize;

  volatile bool signalTable[32];
};

typedef bool (*EventHandler)(void* context, event_t ev);
void EventBus_Signal(EventBus* self, event_t ev);
void EventBus_Tick(EventBus* self, EventHandler handler, void* context);

#endif	/* EVENTQUEUE_H */

