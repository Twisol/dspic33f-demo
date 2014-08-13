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

uint8_t CircleBuffer_Write(CircleBuffer* self, const uint8_t* buf, uint8_t size);
uint8_t CircleBuffer_Read(CircleBuffer* self, uint8_t* buf, uint8_t size);

uint8_t CircleBuffer_Count(CircleBuffer* self);
bool CircleBuffer_IsEmpty(CircleBuffer* self);
bool CircleBuffer_IsFull(CircleBuffer* self);

#endif	/* CIRCLEBUFFER_H */

