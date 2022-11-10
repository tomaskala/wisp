#ifndef WISP_MEMORY_H
#define WISP_MEMORY_H

#include <stdlib.h>

#define ALLOCATE(type, count) wisp_realloc(NULL, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_EXP_CAPACITY(exp) ((exp) < 3 ? 3 : (exp) + 1)

#define GROW_ARRAY(type, ptr, new_count) \
  wisp_realloc(ptr, sizeof(type) * new_count)

#define FREE(ptr) wisp_realloc(ptr, 0)

void *wisp_realloc(void *, size_t);

void *wisp_calloc(size_t, size_t);

#endif
