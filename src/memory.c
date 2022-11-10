#include "memory.h"

void *wisp_realloc(void *ptr, size_t new_size)
{
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL)
    exit(1);

  return result;
}

void *wisp_calloc(size_t nmemb, size_t size)
{
  void *result = calloc(nmemb, size);
  if (result == NULL)
    exit(1);

  return result;
}
