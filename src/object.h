#ifndef WISP_OBJECT_H
#define WISP_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

#define OBJ_TYPE(value)   (AS_OBJ(value)->type)

#define IS_ATOM(value)    is_obj_type(value, OBJ_ATOM)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_LAMBDA(value)  is_obj_type(value, OBJ_LAMBDA)
#define IS_UPVALUE(value) is_obj_type(value, OBJ_UPVALUE)
#define IS_CELL(value)    is_obj_type(value, OBJ_CELL)

#define AS_ATOM(value)    ((struct obj_string *)  AS_OBJ(value))
#define AS_CLOSURE(value) ((struct obj_closure *) AS_OBJ(value))
#define AS_LAMBDA(value)  ((struct obj_lambda *)  AS_OBJ(value))
#define AS_UPVALUE(value) ((struct obj_upvalue *) AS_OBJ(value))
#define AS_CELL(value)    ((struct obj_cell *)    AS_OBJ(value))

// TODO: Switch to #defined constants with a particular meaning?
// TODO: Motivation: Fast tests using bit patterns if needed,
// TODO: visual distinction between string types for atoms & real strings.
enum obj_type {
  OBJ_ATOM,
  OBJ_CLOSURE,
  OBJ_LAMBDA,
  OBJ_UPVALUE,
  OBJ_CELL,
};

struct obj {
  enum obj_type type;
};

static inline bool is_obj_type(Value value, enum obj_type type)
{
  return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

// TODO: Switch to flexible array members.
struct obj_string {
  struct obj obj;
  char *chars;
  size_t len;
  uint64_t hash;
};

struct obj_closure {
  struct obj obj;
  struct obj_lambda *lambda;
  struct obj_upvalue **upvalues;
  int upvalue_count;
};

struct obj_lambda {
  struct obj obj;
  int arity;
  int upvalue_count;
  bool has_param_list;
  struct chunk chunk;
};

struct obj_upvalue {
  struct obj obj;
  Value *location;
  Value closed;
  struct obj_upvalue *next;
};

struct obj_cell {
  struct obj obj;
  Value *car;
  Value *cdr;
};

struct obj_string *copy_string(enum obj_type, const char *, size_t, uint64_t);

struct obj_closure *new_closure(struct obj_lambda *);

struct obj_lambda *new_lambda();

struct obj_upvalue *new_upvalue(Value *);

struct obj_cell *new_cell(Value *, Value *);

#endif
