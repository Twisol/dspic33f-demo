#include "LED.h"

#include <stdint.h>

void LED_Toggle(uint8_t bitvector) {
  RawComm_LED_Toggle(bitvector);
}
