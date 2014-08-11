#include "EventBus.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void EventBus_Init(EventBus* self) {
  memset(self, 0, sizeof(EventBus));
}

void EventBus_Tick(EventBus* self) {
  uint8_t i;
  for (i = 0; i < 32; ++i) {
    if (self->signalTable[i]) {
      EventHandler hook = self->hookTable[i];

      Event event = self->eventTable[i];
      self->eventTable[i].ptr = 0;
      self->signalTable[i] = false;

      if (hook) {
        hook(event);
      }
    }
  }
}

void EventBus_Signal(EventBus* self, uint8_t type, Event event) {
  if (self->signalTable[type]) {
    return; // Already signalled...
  }

  self->signalTable[type] = true;
  self->eventTable[type] = event;
}

void EventBus_SetHook(EventBus* self, uint8_t type, EventHandler handler) {
  self->hookTable[type] = handler;
}