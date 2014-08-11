#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>
#include <stdbool.h>

typedef union Event {
  void* ptr;

  uint16_t uint16;
  uint8_t uint8;

  int16_t int16;
  int8_t int8;
} Event;

typedef void (*EventHandler)(Event event);

typedef struct EventBus {
  EventHandler hookTable[32];
  bool signalTable[32];
  Event eventTable[32];
} EventBus;

void EventBus_Signal(EventBus* self, uint8_t type, Event event);
void EventBus_SetHook(EventBus* self, uint8_t type, EventHandler handler);
void EventBus_Tick(EventBus* self);

#endif	/* EVENTQUEUE_H */

