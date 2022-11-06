#ifndef WISP_CHUNK_H
#define WISP_CHUNK_H

#include <stdlib.h>

#include "common.h"
#include "value.h"

enum opcode {
  OP_CONSTANT,
  OP_NIL,
  OP_CALL,
  OP_DOT_CALL,
  OP_CONS,
  OP_CAR,
  OP_CDR,
  OP_DEFINE_GLOBAL,
  OP_CLOSURE,
  OP_GET_LOCAL,
  OP_GET_UPVALUE,
  OP_GET_GLOBAL,
  OP_RETURN,
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
