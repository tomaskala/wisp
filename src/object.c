#include <string.h>

#include "memory.h"
#include "object.h"

#define ALLOCATE_OBJ(type, obj_type) \
  (type *) allocate_obj(sizeof(type), obj_type)

static struct obj *allocate_obj(size_t size, enum obj_type type)
{
  struct obj *obj = wisp_realloc(NULL, size);
  obj->type = type;
  return obj;
}

static struct obj_string *allocate_string(enum obj_type str_type, char *chars,
    size_t len, uint64_t hash)
{
  struct obj_string *str = ALLOCATE_OBJ(struct obj_string, str_type);
  str->chars = chars;
  str->len = len;
  str->hash = hash;
  return str;
}

struct obj_string *copy_string(enum obj_type str_type, const char *chars,
    size_t len, uint64_t hash)
{
  char *heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0';
  return allocate_string(str_type, heap_chars, len, hash);
}

struct obj_lambda *new_lambda()
{
  struct obj_lambda *lambda = ALLOCATE_OBJ(struct obj_lambda, OBJ_LAMBDA);
  lambda->arity = 0;
  lambda->upvalue_count = 0;
  lambda->has_param_list = false;
  chunk_init(&lambda->chunk);
  return lambda;
}
