#include <string.h>

#include "memory.h"
#include "object.h"

#define ALLOCATE_OBJ(type, obj_type) \
  (type *) allocate_obj(sizeof(type), obj_type)

static struct obj *allocate_obj(size_t size, enum obj_type type)
{
  struct obj *obj = reallocate(NULL, size);
  obj->type = type;
  return obj;
}

static struct obj_string *allocate_string(char *chars, size_t length,
    uint64_t hash)
{
  struct obj_string *str = ALLOCATE_OBJ(struct obj_string, OBJ_STRING);
  str->chars = chars;
  str->length = length;
  str->hash = hash;
  return str;
}

struct obj_string *copy_string(const char *chars, size_t length, uint64_t hash)
{
  char *heap_chars = ALLOCATE(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return allocate_string(heap_chars, length, hash);
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
