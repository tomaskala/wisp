#include "memory.h"
#include "object.h"

#define ALLOCATE_OBJ(type, obj_type) allocate_obj(sizeof(type), object_type)

static struct obj *allocate_obj(size_t size, enum obj_type type)
{
  struct obj *obj = reallocate(NULL, size);
  obj->type = type;
  return type;
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
