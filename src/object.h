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

#define AS_ATOM(value)    ((struct obj_string *)  AS_OBJ(value))
#define AS_CLOSURE(value) ((struct obj_closure *) AS_OBJ(value))
#define AS_LAMBDA(value)  ((struct obj_lambda *)  AS_OBJ(value))
#define AS_UPVALUE(value) ((struct obj_upvalue *) AS_OBJ(value))

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

static inline bool is_obj_type(Value value, enum obj_type type)
{
  return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

// TODO: Switch to flexible array members.
struct obj_string {
  struct obj obj;

  // Null-terminated bytes of the string.
  char *chars;

  // Number of bytes in the string.
  size_t len;

  // Hash of the string contents.
  uint64_t hash;
};

struct obj_closure {
  struct obj obj;

  // The lambda this closure is an instance of.
  struct obj_lambda *lambda;

  // The upvalues this closure has closed over.
  struct obj_upvalue **upvalues;

  // Number of upvalues closed over.
  int upvalue_count;  // TODO: Remove? Accessible from obj_lambda.
};

struct obj_lambda {
  struct obj obj;

  // Number of arguments the lambda expects. If 'has_param_list' is true, this
  // includes the parameter list.
  int arity;

  // Number of upvalues closed over.
  int upvalue_count;

  // Whether the lambda accepts an arbitrary number of arguments to be
  // collected in a list.
  bool has_param_list;

  // Bytecode of the lambda body.
  struct chunk chunk;
};

struct obj_upvalue {
  struct obj obj;

  // Pointer to the variable this upvalue is referencing. Either its location
  // on the stack (if the upvalue is open), or the 'closed' field (if the
  // upvalue is closed).
  Value *location;

  // If the upvalue is closed, the value of the referenced variable is moved
  // here from the stack.
  Value closed;

  // Open upvalues are stored in a linked list.
  struct obj_upvalue *next;
};

struct obj_string *string_copy(enum obj_type, const char *, size_t, uint64_t);

struct obj_closure *closure_new(struct obj_lambda *);

struct obj_lambda *lambda_new();

struct obj_upvalue *upvalue_new(Value *);

void object_print(Value);

#endif
