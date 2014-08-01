#include "LED.h"

#include <stdint.h>

extern void RawComm_LED_Init();
extern void RawComm_LED_Toggle(uint8_t bitvector);

void LED_Init() {
  RawComm_LED_Init();
}

void LED_Toggle(uint8_t bitvector) {
  RawComm_LED_Toggle(bitvector);
}
