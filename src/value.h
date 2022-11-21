#ifndef WISP_VALUE_H
#define WISP_VALUE_H

#include <stdlib.h>

enum value_type {
  VAL_NIL,
  VAL_NUM,
  VAL_CONS,
  VAL_OBJ,
};

typedef struct {
  enum value_type type;
  union {
    double number;
    int hp;
    struct obj *obj;
  } as;
} Value;

#define IS_NIL(value)  ((value).type == VAL_NIL)
#define IS_NUM(value)  ((value).type == VAL_NUM)
#define IS_OBJ(value)  ((value).type == VAL_OBJ)
#define IS_CONS(value) ((value).type == VAL_CONS)

#define AS_NUM(value)  ((value).as.number)
#define AS_OBJ(value)  ((value).as.obj)
#define AS_CONS(value) ((value).as.hp)
#define AS_CAR(value)  AS_CONS(value)
#define AS_CDR(value)  (AS_CONS(value) + 1)

#define NIL_VAL        ((Value) {VAL_NIL,  {.number = 0}})
#define NUM_VAL(n)     ((Value) {VAL_NUM,  {.number = (n)}})
#define OBJ_VAL(o)     ((Value) {VAL_OBJ,  {.obj = (struct obj *) (o)}})
#define CONS_VAL(h)    ((Value) {VAL_CONS, {.hp = (h)}})

void value_print(Value);

struct value_array {
  int capacity;
  int count;
  Value *values;
};

void value_array_init(struct value_array *);

void value_array_write(struct value_array *, Value);

void value_array_free(struct value_array *);

#endif
