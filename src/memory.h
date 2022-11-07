#ifndef WISP_MEMORY_H
#define WISP_MEMORY_H

#include <stdlib.h>

#define ALLOCATE(type, count) reallocate(NULL, sizeof(type) * (count))

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, ptr, new_count) \
  reallocate(ptr, sizeof(type) * new_count)

#define FREE(ptr) reallocate(ptr, 0)

void *reallocate(void *, size_t);

#endif
