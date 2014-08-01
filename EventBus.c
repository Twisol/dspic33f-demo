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


void EventBus_Tick() {
  uint8_t i;
  for (i = 0; i < 32; ++i) {
    if (signalTable[i]) {
      signalTable[i] = false;

      if (hookTable[i]) {
        hookTable[i]();
      }
    }
  }
}

void EventBus_Signal(uint8_t type) {
  signalTable[type] = true;
}

void EventBus_SetHook(uint8_t type, EventHandler handler) {
  hookTable[type] = handler;
}