#include "memory.h"

void *wisp_realloc(void *ptr, size_t old_size, size_t new_size)
{
  (void) old_size;  // TODO

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL)
    exit(1);

  return result;
}

void *wisp_calloc(size_t old_nmemb, size_t new_nmemb, size_t size)
{
  (void) old_nmemb;  // TODO

  void *result = calloc(new_nmemb, size);
  if (result == NULL)
    exit(1);

  return result;
}
