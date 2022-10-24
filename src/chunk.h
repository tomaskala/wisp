#ifndef LISP_CHUNK_H
#define LISP_CHUNK_H

#include <stdint.h>
#include <stdlib.h>

#include "value.h"

enum opcode {
  OP_CONSTANT,
  OP_NIL,
  OP_QUOTE,
  OP_CALL,
  OP_DOT_CALL,
  OP_CONS,
  OP_CAR,
  OP_CDR,
};

struct chunk {
  size_t capacity;
  size_t count;
  uint8_t *code;
  int *lines;  // TODO: Use run-length encoding.
  struct value_array constants;
};

void chunk_init(struct chunk *);

void chunk_write(struct chunk *, uint8_t, int);

int chunk_add_constant(struct chunk *, Value);

void chunk_free(struct chunk *);

#endif
