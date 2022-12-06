#ifndef WISP_VALUE_H
#define WISP_VALUE_H

#include "common.h"

// Primitive values.
// ============================================================================

enum value_type {
  VAL_NIL,
  VAL_NUM,
  VAL_OBJ,
};

typedef struct {
  enum value_type type;
  union {
    double number;
    struct obj *obj;
  } as;
} Value;

#define IS_NIL(value)  ((value).type == VAL_NIL)
#define IS_NUM(value)  ((value).type == VAL_NUM)
#define IS_OBJ(value)  ((value).type == VAL_OBJ)

#define AS_NUM(value)  ((value).as.number)
#define AS_OBJ(value)  ((value).as.obj)

#define NIL_VAL        ((Value) {VAL_NIL,  {.number = 0}})
#define NUM_VAL(n)     ((Value) {VAL_NUM,  {.number = (n)}})
#define OBJ_VAL(o)     ((Value) {VAL_OBJ,  {.obj = (struct obj *) (o)}})

void value_print(Value);

struct value_array {
  int capacity;
  int count;
  Value *values;
};

void value_array_init(struct value_array *);

void value_array_write(struct wisp_state *, struct value_array *, Value);

void value_array_free(struct wisp_state *, struct value_array *);

// Chunk type.
// ============================================================================

struct chunk {
  int capacity;
  int count;
  uint8_t *code;
  int *lines;  // TODO: Use run-length encoding.
  struct value_array constants;
};

void chunk_init(struct chunk *);

void chunk_write(struct wisp_state *, struct chunk *, uint8_t, int);

int chunk_add_constant(struct wisp_state *, struct chunk *, Value);

void chunk_free(struct wisp_state *, struct chunk *);

// Heap-allocated objects.
// ============================================================================

#define OBJ_TYPE(value)   (AS_OBJ(value)->type)

#define IS_ATOM(value)    is_obj_type(value, OBJ_ATOM)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_LAMBDA(value)  is_obj_type(value, OBJ_LAMBDA)
#define IS_UPVALUE(value) is_obj_type(value, OBJ_UPVALUE)
#define IS_PAIR(value)    is_obj_type(value, OBJ_PAIR)

#define AS_ATOM(value)    ((struct obj_string *)  AS_OBJ(value))
#define AS_CLOSURE(value) ((struct obj_closure *) AS_OBJ(value))
#define AS_LAMBDA(value)  ((struct obj_lambda *)  AS_OBJ(value))
#define AS_UPVALUE(value) ((struct obj_upvalue *) AS_OBJ(value))
#define AS_PAIR(value)    ((struct obj_pair *)    AS_OBJ(value))

// TODO: Switch to #defined constants with a particular meaning?
// TODO: Motivation: Fast tests using bit patterns if needed,
// TODO: visual distinction between string types for atoms & real strings.
enum obj_type {
  OBJ_ATOM,
  OBJ_CLOSURE,
  OBJ_LAMBDA,
  OBJ_UPVALUE,
  OBJ_PAIR,
};

struct obj {
  enum obj_type type;
  bool is_marked;
  struct obj *next;
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

struct obj_pair {
  struct obj obj;

  // The first element of the pair.
  Value car;

  // The second element of the pair.
  Value cdr;
};

struct obj_string *string_copy(struct wisp_state *, enum obj_type,
    const char *, size_t, uint64_t);

struct obj_closure *closure_new(struct wisp_state *, struct obj_lambda *);

struct obj_lambda *lambda_new(struct wisp_state *);

struct obj_upvalue *upvalue_new(struct wisp_state *, Value *);

struct obj_pair *pair_new(struct wisp_state *, Value, Value);

void obj_print(Value);

#endif
