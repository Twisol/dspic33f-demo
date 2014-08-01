#ifndef RAWCOMM_H
#define	RAWCOMM_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*PushButtonHandler)();

void RawComm_LCD_Init();
void RawComm_LCD_Send(bool is_data, bool do_read, uint64_t delay, uint16_t payload);

void RawComm_LED_Init();
void RawComm_LED_Toggle(uint8_t bitvector);

void RawComm_PushButton_Init(PushButtonHandler hook);

#endif	/* RAWCOMM_H */

