#ifndef EVENTQUEUE_H
#define	EVENTQUEUE_H

#include <stdint.h>

typedef union Event {
  void* ptr;

  uint16_t uint16;
  uint8_t uint8;

  int16_t int16;
  int8_t int8;
} Event;

typedef void (*EventHandler)(Event event);

void EventBus_Signal(uint8_t type, Event event);
void EventBus_SetHook(uint8_t type, EventHandler handler);
void EventBus_Tick();

#endif	/* EVENTQUEUE_H */

