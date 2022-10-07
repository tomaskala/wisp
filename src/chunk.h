#ifndef LISP_CHUNK_H
#define LISP_CHUNK_H

#include <stdint.h>
#include <stdlib.h>

#include "value.h"

struct chunk {
  size_t capacity;
  size_t count;
  uint8_t *code;
  int *lines;  // TODO: Use run-length encoding.
  struct value_array constants;
};

void chunk_init(struct chunk *);

void chunk_write(struct chunk *, uint8_t, int);

int chunk_add_constant(struct chunk *, value);

void chunk_free(struct chunk *);

#endif
