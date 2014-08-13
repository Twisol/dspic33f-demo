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

uint8_t CircleBuffer_Write(CircleBuffer* self, const uint8_t* buf, uint8_t size) {
  uint8_t idx = 0;
  while (idx < size) {
    if (CircleBuffer_IsFull(self)) {
      break;
    }

    self->data[self->write_offset] = buf[idx];

    self->write_offset = (self->write_offset + 1) % self->length;
    idx += 1;
  }

  return idx; // bytes written
}

uint8_t CircleBuffer_Read(CircleBuffer* self, uint8_t* buf, uint8_t size) {
  uint8_t idx = 0;
  while (idx < size) {
    if (CircleBuffer_IsEmpty(self)) {
      break;
    }

    buf[idx] = self->data[self->read_offset];

    self->read_offset = (self->read_offset + 1) % self->length;
    idx += 1;
  }

  return idx; // bytes read
}

uint8_t CircleBuffer_Count(CircleBuffer* self) {
  return (self->write_offset + self->length - self->read_offset) % self->length;
}

bool CircleBuffer_IsEmpty(CircleBuffer* self) {
  return (self->write_offset == self->read_offset);
}

bool CircleBuffer_IsFull(CircleBuffer* self) {
  return ((self->write_offset + 1) % self->length == self->read_offset);
}
