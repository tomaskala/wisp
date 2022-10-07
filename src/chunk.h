#ifndef LISP_CHUNK_H
#define LISP_CHUNK_H

#include <stdint.h>
#include <stdlib.h>

struct chunk {
  size_t capacity;
  size_t count;
  uint8_t *code;
  int *lines;  // TODO: Use run-length encoding.
};

void chunk_init(struct chunk *);

void chunk_write(struct chunk *, uint8_t, int);

void chunk_free(struct chunk *);

#endif
