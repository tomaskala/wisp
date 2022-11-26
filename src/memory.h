#ifndef WISP_MEMORY_H
#define WISP_MEMORY_H

#include <stdlib.h>

#include "state.h"

#define ALLOCATE(type, count) wisp_realloc(NULL, 0, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_EXP_CAPACITY(exp) ((exp) < 3 ? 3 : (exp) + 1)

#define GROW_ARRAY(type, ptr, old_count, new_count) \
  wisp_realloc(ptr, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE(type, ptr) wisp_realloc(ptr, sizeof(type), 0)

#define FREE_ARRAY(type, ptr, old_count) \
  wisp_realloc(ptr, sizeof(type) * (old_count), 0)

void *wisp_realloc(void *, size_t, size_t);

void *wisp_calloc(struct wisp_state *, size_t, size_t, size_t);

#endif
