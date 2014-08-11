#ifndef CIRCLEBUFFER_H
#define CIRCLEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct CircleBuffer {
  uint8_t length;
  uint8_t read_offset;
  uint8_t write_offset;

  uint8_t data[128];
} CircleBuffer;

void CircleBuffer_Init(CircleBuffer* self);
bool CircleBuffer_Push(CircleBuffer* self, uint8_t ch);
bool CircleBuffer_Pop(CircleBuffer* self, uint8_t* ch);
uint8_t CircleBuffer_Count(CircleBuffer* self);

#endif	/* CIRCLEBUFFER_H */

