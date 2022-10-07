#ifndef LISP_MEMORY_H
#define LISP_MEMORY_H

#include <stdlib.h>

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(type, ptr, new_count) \
  reallocate(ptr, sizeof(type) * new_count)

#define FREE(ptr) reallocate(ptr, 0)

void *reallocate(void *, size_t);

#endif
