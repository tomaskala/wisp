#include <string.h>

#include "memory.h"
#include "state.h"
#include "strpool.h"

#define CAPACITY(exp) (1 << (exp))

static int32_t ht_lookup(uint64_t hash, int exp, int32_t idx)
{
  uint32_t mask = ((uint32_t) 1 << exp) - 1;
  uint32_t step = (hash >> (64 - exp)) | 1;
  return (idx + step) & mask;
}

// FNV-1a
// TODO: Benchmark, experiment with the hash Lua uses (lstring.c:luaS_hash)
static uint64_t hash_string(const char *str, size_t len)
{
  uint64_t hash = 0x3243f6a8885a308d;

  for (; len > 0; --len) {
    hash ^= str[len - 1] & 255;
    hash *= 1111111111111111111;
  }

  return hash ^ hash >> 32;
}

static void adjust_capacity(struct wisp_state *w)
{
  struct str_pool *pool = &w->str_pool;
  size_t old_capacity = (size_t) (pool->ht == NULL ? 0 : CAPACITY(pool->exp));

  int new_count = 0;
  int new_exp = GROW_EXP_CAPACITY(pool->exp);
  struct obj_string **new_ht = wisp_calloc(w, old_capacity,
      (size_t) CAPACITY(new_exp), sizeof(struct obj_string *));

  if (pool->ht != NULL) {
    for (int p = 0; p < CAPACITY(pool->exp); ++p) {
      if (pool->ht[p] == NULL || pool->ht[p] == &pool->gravestone)
        continue;

      uint64_t hash = pool->ht[p]->hash;

      for (int32_t i = hash;;) {
        i = ht_lookup(hash, new_exp, i);

        if (new_ht[i] == NULL) {
          new_count++;
          new_ht[i] = pool->ht[p];
          break;
        }
      }
    }
  }

  FREE_ARRAY(w, struct obj_string *, pool->ht, old_capacity);
  pool->exp = new_exp;
  pool->count = new_count;
  pool->ht = new_ht;
}

void str_pool_init(struct wisp_state *w)
{
  struct str_pool *pool = &w->str_pool;

  // The placeholder value is allocated on the stack and initialized
  // to marked = true. That way, it is never collected, and can be
  // referenced from the table when an element is removed.
  pool->gravestone.obj.type = OBJ_ATOM;
  pool->gravestone.obj.is_marked = true;
  pool->gravestone.obj.next = NULL;
  pool->gravestone.chars = "<deleted>";
  pool->gravestone.len = strlen(pool->gravestone.chars);
  pool->gravestone.hash = (uint64_t) 0;

  // Together with resizing at 50% load, initializing exp to 1
  // ensures that the array gets allocated upon first interning.
  pool->exp = 1;
  pool->count = 0;
  pool->ht = NULL;
}

struct obj_string *str_pool_intern(struct wisp_state *w, const char *str,
    size_t len)
{
  struct str_pool *pool = &w->str_pool;
  struct obj_string **dest = NULL;

  if (pool->count + 1 == CAPACITY(pool->exp - 1))
    // Resize at 50% load.
    adjust_capacity(w);

  uint64_t hash = hash_string(str, len);

  for (int32_t i = hash;;) {
    i = ht_lookup(hash, pool->exp, i);

    if (pool->ht[i] == NULL) {
      if (dest == NULL) {
        // Inserting an element to a previously unoccupied position, as
        // opposed to one previously occupied by a deleted element and
        // now filled with the placeholder value.
        pool->count++;
        dest = &pool->ht[i];
      }

      // TODO: Once strings are implemented, change the object type.
      *dest = string_copy(w, OBJ_ATOM, str, len, hash);
      return *dest;
    } else if (dest == NULL && pool->ht[i] == &pool->gravestone) {
      dest = &pool->ht[i];
    } else if (pool->ht[i]->len == len
        && memcmp(pool->ht[i]->chars, str, len) == 0) {
      return pool->ht[i];
    }
  }
}

void str_pool_unintern(struct wisp_state *w, struct obj_string *str)
{
  struct str_pool *pool = &w->str_pool;

  for (int32_t i = str->hash;;) {
    i = ht_lookup(str->hash, pool->exp, i);

    if (pool->ht[i] == NULL)
      return;

    if (pool->ht[i] != &pool->gravestone
        && pool->ht[i]->len == str->len
        && memcmp(pool->ht[i]->chars, str->chars, str->len) == 0) {
      pool->ht[i] = &pool->gravestone;
      return;
    }
  }
}

void str_pool_remove_white(struct wisp_state *w)
{
  struct str_pool *pool = &w->str_pool;

  for (int i = 0; i < CAPACITY(pool->exp); ++i) {
    if (pool->ht[i] != NULL && !pool->ht[i]->obj.is_marked)
      str_pool_unintern(w, pool->ht[i]);
  }
}

void str_pool_free(struct wisp_state *w)
{
  struct str_pool *pool = &w->str_pool;
  size_t old_capacity = (size_t) (pool->ht == NULL ? 0 : CAPACITY(pool->exp));
  FREE_ARRAY(w, struct obj_string *, pool->ht, old_capacity);
  str_pool_init(w);
}
