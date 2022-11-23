#include <stdio.h>
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

struct obj_string *string_copy(enum obj_type str_type, const char *chars,
    size_t len, uint64_t hash)
{
  char *heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0';
  return allocate_string(str_type, heap_chars, len, hash);
}

struct obj_closure *closure_new(struct obj_lambda *lambda)
{
  struct obj_upvalue **upvalues = ALLOCATE(struct obj_upvalue *,
      lambda->upvalue_count);

  for (int i = 0; i < lambda->upvalue_count; ++i)
    upvalues[i] = NULL;

  struct obj_closure *closure = ALLOCATE_OBJ(struct obj_closure, OBJ_CLOSURE);
  closure->lambda = lambda;
  closure->upvalues = upvalues;
  closure->upvalue_count = lambda->upvalue_count;
  return closure;
}

struct obj_lambda *lambda_new()
{
  struct obj_lambda *lambda = ALLOCATE_OBJ(struct obj_lambda, OBJ_LAMBDA);
  lambda->arity = 0;
  lambda->upvalue_count = 0;
  lambda->has_param_list = false;
  chunk_init(&lambda->chunk);
  return lambda;
}

struct obj_upvalue *upvalue_new(Value *slot)
{
  struct obj_upvalue *upvalue = ALLOCATE_OBJ(struct obj_upvalue, OBJ_UPVALUE);
  upvalue->location = slot;
  upvalue->closed = NIL_VAL;
  upvalue->next = NULL;
  return upvalue;
}

struct obj_pair *pair_new(Value car, Value cdr)
{
  struct obj_pair *pair = ALLOCATE_OBJ(struct obj_pair, OBJ_PAIR);
  pair->car = car;
  pair->cdr = cdr;
  return pair;
}

void object_print(Value val)
{
  switch (OBJ_TYPE(val)) {
  case OBJ_ATOM:
    printf("%s", AS_ATOM(val)->chars);
    break;
  case OBJ_CLOSURE:
    printf("closure");
    break;
  case OBJ_LAMBDA:
    printf("lambda");
    break;
  case OBJ_UPVALUE:
    printf("upvalue");
    break;
  case OBJ_PAIR: {
    for (putchar('(');; putchar(' ')) {
      struct obj_pair *pair = AS_PAIR(val);
      value_print(pair->car);
      val = pair->cdr;

      if (IS_NIL(val))
        break;
      else if (!IS_PAIR(val)) {
        printf(" . ");
        value_print(val);
        break;
      }
    }

    putchar(')');
    break;
  }
  }
}
