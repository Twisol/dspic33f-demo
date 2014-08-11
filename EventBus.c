#include "EventBus.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void EventBus_Init(EventBus* self) {
  memset(self, 0, sizeof(EventBus));
}

void EventBus_Tick(EventBus* self, EventHandler handler, void* context) {
  uint8_t i;
  for (i = 0; i < 32; ++i) {
    if (self->signalTable[i]) {
      self->signalTable[i] = false;
      handler(context, i);
    }
  }
}

void EventBus_Signal(EventBus* self, uint8_t type) {
  self->signalTable[type] = true;
}