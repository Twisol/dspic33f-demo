#ifndef LED_H
#define	LED_H

#include <stdint.h>

// Imported API
void RawComm_LED_Toggle(uint8_t bitvector);

// Exported API
void LED_Toggle(uint8_t bitvector);

#endif	/* LED_H */

