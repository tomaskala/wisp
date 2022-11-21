#include <stdio.h>

#include "debug.h"
#include "object.h"
#include "value.h"

static int constant_instruction(const char *name, struct chunk *chunk,
    int offset)
{
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4u '", name, constant);
  value_print(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int simple_instruction(const char *name, int offset)
{
  printf("%s\n", name);
  return offset + 1;
}

static int byte_instruction(const char *name, struct chunk *chunk, int offset)
{
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);
  return offset + 2;
}

int disassemble_instruction(struct chunk *chunk, int offset)
{
  printf("%04d ", offset);

  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    printf("   | ");
  else
    printf("%4d ", chunk->lines[offset]);

  uint8_t instruction = chunk->code[offset];

  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_NIL:
    return simple_instruction("OP_NIL", offset);
  case OP_CALL:
    return byte_instruction("OP_CALL", chunk, offset);
  case OP_DOT_CALL:
    return byte_instruction("OP_DOT_CALL", chunk, offset);
  case OP_CLOSURE: {
    offset++;
    uint8_t constant = chunk->code[offset++];
    printf("%-16s %4d ", "OP_CLOSURE", constant);
    value_print(chunk->constants.values[constant]);
    printf("\n");

    struct obj_lambda *lambda = AS_LAMBDA(chunk->constants.values[constant]);

    for (int i = 0; i < lambda->upvalue_count; ++i) {
      uint8_t is_local = chunk->code[offset++];
      uint8_t index = chunk->code[offset++];
      printf("%04d | %s %d\n", offset - 2, is_local ? "local" : "upvalue",
          index);
    }

    return offset;
  }
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_CONS:
    return simple_instruction("CONS", offset);
  case OP_CAR:
    return simple_instruction("CAR", offset);
  case OP_CDR:
    return simple_instruction("CDR", offset);
  case OP_DEFINE_GLOBAL:
    return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return byte_instruction("OP_GET_LOCAL", chunk, offset);
  case OP_GET_UPVALUE:
    return byte_instruction("OP_GET_UPVALUE", chunk, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("OP_GET_GLOBAL", chunk, offset);
  default:
    printf("Unknown opcode: %" PRIu8 "\n", instruction);
    return offset + 1;
  }
}
