#include "EventBus.h"

#include <stdbool.h>
#include <stdint.h>

static EventHandler hookTable[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};
static bool signalTable[32] = {
  false, false, false, false, false, false, false, false,
  false, false, false, false, false, false, false, false,
  false, false, false, false, false, false, false, false,
  false, false, false, false, false, false, false, false,
};
static Event eventTable[32] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
};

void EventBus_Init() {
}

void EventBus_Tick() {
  uint8_t i;
  for (i = 0; i < 32; ++i) {
    if (signalTable[i]) {
      EventHandler hook = hookTable[i];

      Event event = eventTable[i];
      eventTable[i].ptr = 0;
      signalTable[i] = false;

      if (hook) {
        hook(event);
      }
    }
  }
}

void EventBus_Signal(uint8_t type, Event event) {
  if (signalTable[type]) {
    return; // Already signalled...
  }

  signalTable[type] = true;
  eventTable[type] = event;
}

void EventBus_SetHook(uint8_t type, EventHandler handler) {
  hookTable[type] = handler;
}