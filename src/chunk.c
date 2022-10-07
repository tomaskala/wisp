#include "chunk.h"
#include "memory.h"

void chunk_init(struct chunk *chunk)
{
  chunk->capacity = 0;
  chunk->count = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
}

void chunk_write(struct chunk *chunk, uint8_t codepoint, int line)
{
  if (chunk->count >= chunk->capacity) {
    chunk->capacity = GROW_CAPACITY(chunk->capacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->capacity);
    chunk->lines = GROW_ARRAY(int, chunk->lines, chunk->capacity);
  }

  chunk->code[chunk->count] = codepoint;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

void chunk_free(struct chunk *chunk)
{
  FREE(chunk->code);
  FREE(chunk->lines);
  chunk_init(chunk);
}
