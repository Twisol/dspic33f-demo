#include "CircleBuffer.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void CircleBuffer_Init(CircleBuffer* self) {
  memset((uint8_t*)self->data, 0, 128*sizeof(uint8_t));
  self->read_offset = 0;
  self->write_offset = 0;
  self->length = 128;
}

bool CircleBuffer_Push(CircleBuffer* self, uint8_t ch) {
  uint8_t next_write = (self->write_offset + 1) % self->length;
  if (next_write == self->read_offset) {
    return false;
  }

  self->data[self->write_offset] = ch;
  self->write_offset = next_write;

  return true;
}

bool CircleBuffer_Pop(CircleBuffer* self, uint8_t* ch) {
  if (self->read_offset == self->write_offset) {
    return false;
  }

  *ch = self->data[self->read_offset];
  self->read_offset = (self->read_offset + 1) % self->length;

  return true;
}

uint8_t CircleBuffer_Count(CircleBuffer* self) {
  return (self->write_offset + self->length - self->read_offset) % self->length;
}