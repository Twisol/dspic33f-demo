#include "EventBus.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void EventBus_Init(EventBus* self, EventBus* master, event_t masterEvent) {
  memset(self, 0, sizeof(EventBus));
  self->master = master;
  self->masterEvent = masterEvent;
  self->tableSize = 32;
}

void EventBus_Tick(EventBus* self, EventHandler handler, void* context) {
  uint8_t i;
  for (i = 0; i < self->tableSize; ++i) {
    if (self->signalTable[i]) {
      self->signalTable[i] = false;
      handler(context, i);
    }
  }
}

void EventBus_Signal(EventBus* self, uint8_t type) {
  self->signalTable[type] = true;

  if (self->master != NULL) {
    EventBus_Signal(self->master, self->masterEvent);
  }
}

extern inline mailbox_t mailbox(EventBus* target, event_t event);
extern inline void Mailbox_Deliver(mailbox_t mailbox);
