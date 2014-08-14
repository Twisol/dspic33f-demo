#ifndef PUSHBUTTONS_H
#define PUSHBUTTONS_H

#include "../EventBus.h"

typedef struct ButtonInterface {
  EventBus* bus;
  uint8_t evt_CHANGE;
} ButtonInterface;

#endif	/* PUSHBUTTONS_H */

