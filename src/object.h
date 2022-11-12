#ifndef WISP_OBJECT_H
#define WISP_OBJECT_H

#include "chunk.h"
#include "common.h"
#include "value.h"

// TODO: Switch to #defined constants with a particular meaning?
// TODO: Motivation: Fast tests using bit patterns if needed,
// TODO: visual distinction between string types for atoms & real strings.
enum obj_type {
  OBJ_ATOM,
  OBJ_CLOSURE,
  OBJ_LAMBDA,
  OBJ_UPVALUE,
};

struct obj {
  enum obj_type type;
};

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

struct obj_string *copy_string(enum obj_type, const char *, size_t, uint64_t);

struct obj_closure *new_closure(struct obj_lambda *);

struct obj_lambda *new_lambda();

struct obj_upvalue *new_upvalue(Value *);

#endif
