#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "state.h"
#include "value.h"

// Primitive values.
// ============================================================================

static void object_print(Value val);

void value_print(Value val)
{
  switch (val.type) {
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUM:
    printf("%g", AS_NUM(val));
    break;
  case VAL_OBJ:
    object_print(val);
    break;
  }
}

void value_array_init(struct value_array *array)
{
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void value_array_write(struct wisp_state *w, struct value_array *array,
    Value val)
{
  if (array->count >= array->capacity) {
    int old_capacity = array->capacity;
    array->capacity = GROW_CAPACITY(old_capacity);
    array->values = GROW_ARRAY(w, Value, array->values, old_capacity,
        array->capacity);
  }

  array->values[array->count] = val;
  array->count++;
}

void value_array_free(struct wisp_state *w, struct value_array *array)
{
  FREE_ARRAY(w, Value, array->values, array->capacity);
  value_array_init(array);
}

// Chunk type.
// ============================================================================

void chunk_init(struct chunk *chunk)
{
  chunk->capacity = 0;
  chunk->count = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  value_array_init(&chunk->constants);
}

void chunk_write(struct wisp_state *w, struct chunk *chunk, uint8_t codepoint,
    int line)
{
  if (chunk->count >= chunk->capacity) {
    int old_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(old_capacity);
    chunk->code = GROW_ARRAY(w, uint8_t, chunk->code, old_capacity,
        chunk->capacity);
    chunk->lines = GROW_ARRAY(w, int, chunk->lines, old_capacity,
        chunk->capacity);
  }

  chunk->code[chunk->count] = codepoint;
  chunk->lines[chunk->count] = line;
  chunk->count++;
}

int chunk_add_constant(struct wisp_state *w, struct chunk *chunk,
    Value constant)
{
  value_array_write(w, &chunk->constants, constant);
  return chunk->constants.count - 1;
}

void chunk_free(struct wisp_state *w, struct chunk *chunk)
{
  FREE_ARRAY(w, uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(w, int, chunk->lines, chunk->capacity);
  chunk_init(chunk);
}

// Heap-allocated objects.
// ============================================================================

#define ALLOCATE_OBJ(w, type, obj_type) \
  (type *) allocate_obj(w, sizeof(type), obj_type)

static struct obj *allocate_obj(struct wisp_state *w, size_t size,
    enum obj_type type)
{
  struct obj *obj = wisp_realloc(w, NULL, 0, size);
  obj->type = type;
  obj->next = w->objects;
  w->objects = obj;
#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", (void *) obj, size, type);
#endif
  return obj;
}

static struct obj_string *allocate_string(struct wisp_state *w,
    enum obj_type str_type, char *chars, size_t len, uint64_t hash)
{
  struct obj_string *str = ALLOCATE_OBJ(w, struct obj_string, str_type);
  str->chars = chars;
  str->len = len;
  str->hash = hash;
  return str;
}

struct obj_string *string_copy(struct wisp_state *w, enum obj_type str_type,
    const char *chars, size_t len, uint64_t hash)
{
  char *heap_chars = ALLOCATE(w, char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0';
  return allocate_string(w, str_type, heap_chars, len, hash);
}

struct obj_closure *closure_new(struct wisp_state *w,
    struct obj_lambda *lambda)
{
  struct obj_upvalue **upvalues = ALLOCATE(w, struct obj_upvalue *,
      lambda->upvalue_count);

  for (int i = 0; i < lambda->upvalue_count; ++i)
    upvalues[i] = NULL;

  struct obj_closure *closure = ALLOCATE_OBJ(w, struct obj_closure,
      OBJ_CLOSURE);
  closure->lambda = lambda;
  closure->upvalues = upvalues;
  closure->upvalue_count = lambda->upvalue_count;
  return closure;
}

struct obj_lambda *lambda_new(struct wisp_state *w)
{
  struct obj_lambda *lambda = ALLOCATE_OBJ(w, struct obj_lambda, OBJ_LAMBDA);
  lambda->arity = 0;
  lambda->upvalue_count = 0;
  lambda->has_param_list = false;
  chunk_init(&lambda->chunk);
  return lambda;
}

struct obj_upvalue *upvalue_new(struct wisp_state *w, Value *slot)
{
  struct obj_upvalue *upvalue = ALLOCATE_OBJ(w, struct obj_upvalue,
      OBJ_UPVALUE);
  upvalue->location = slot;
  upvalue->closed = NIL_VAL;
  upvalue->next = NULL;
  return upvalue;
}

struct obj_pair *pair_new(struct wisp_state *w, Value car, Value cdr)
{
  struct obj_pair *pair = ALLOCATE_OBJ(w, struct obj_pair, OBJ_PAIR);
  pair->car = car;
  pair->cdr = cdr;
  return pair;
}

static void object_print(Value val)
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
