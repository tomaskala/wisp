#ifndef LISP_VALUE_H
#define LISP_VALUE_H

#include <stdlib.h>

enum value_type {
  VAL_NIL,
  VAL_NUM,
  VAL_OBJ,
};

typedef struct {
  enum value_type type;
  union {
    double number;
    struct object *obj;
  } as;
} Value;

#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUM(value) ((value).type == VAL_NUM)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_NUM(value) ((value).as.number)
#define AS_OBJ(value) ((value).as.obj)

#define NIL_VAL       ((Value) {VAL_NIL, {.number = 0}})
#define NUM_VAL(n)    ((Value) {VAL_NUM, {.number = n}})
#define OBJ_VAL(o)    ((Value) {VAL_OBJ, {.obj = (struct object *) o}})

struct value_array {
  size_t capacity;
  size_t count;
  Value *values;
};

void value_array_init(struct value_array *);

void value_array_write(struct value_array *, Value);

void value_array_free(struct value_array *);

#endif
