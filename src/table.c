#include <string.h>

#include "common.h"
#include "memory.h"
#include "object.h"
#include "table.h"

#define CAPACITY(exp) (1 << (exp))

static int32_t ht_lookup(uint64_t hash, int exp, int32_t idx)
{
  uint32_t mask = ((uint32_t) 1 << exp) - 1;
  uint32_t step = (hash >> (64 - exp)) | 1;
  return (idx + step) & mask;
}

// FNV-1a
static uint64_t hash_string(const char *str, int length)
{
  uint64_t hash = 0x3243f6a8885a308d;

  for (int i = 0; i < length; ++i) {
    hash ^= str[i] & 255;
    hash *= 1111111111111111111;
  }

  return hash ^ hash >> 32;
}

static void adjust_capacity(struct str_pool *pool)
{
  int new_exp = GROW_EXP_CAPACITY(pool->exp);
  struct obj_string **new_ht = wisp_calloc((size_t) CAPACITY(new_exp),
      sizeof(struct obj_string *));

  if (pool->ht != NULL) {
    for (int p = 0; p < CAPACITY(pool->exp); ++p) {
      if (pool->ht[p] == NULL)
        continue;

      uint64_t hash = pool->ht[p]->hash;

      for (int32_t i = hash;;) {
        i = ht_lookup(hash, new_exp, i);

        if (new_ht[i] == NULL) {
          new_ht[i] = pool->ht[p];
          break;
        }
      }
    }
  }

  FREE(pool->ht);
  pool->exp = new_exp;
  pool->ht = new_ht;
}

void str_pool_init(struct str_pool *pool)
{
  // Together with resizing at 50% load, initializing exp to 1
  // ensures that the array gets allocated upon first interning.
  pool->exp = 1;
  pool->count = 0;
  pool->ht = NULL;
}

void str_pool_free(struct str_pool *pool)
{
  FREE(pool->ht);
  str_pool_init(pool);
}

struct obj_string *intern(struct str_pool *pool, const char *str,
    size_t length)
{
  if (pool->count + 1 == CAPACITY(pool->exp - 1))
    // Resize at 50% load.
    adjust_capacity(pool);

  uint64_t hash = hash_string(str, length);

  for (int32_t i = hash;;) {
    i = ht_lookup(hash, pool->exp, i);

    if (pool->ht[i] == NULL) {
      pool->count++;
      pool->ht[i] = copy_string(str, length, hash);
      return pool->ht[i];
    } else if (pool->ht[i]->length == length
        && memcmp(pool->ht[i]->chars, str, length) == 0)
      return pool->ht[i];
  }
}
