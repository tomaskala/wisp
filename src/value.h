#ifndef LISP_VALUE_H
#define LISP_VALUE_H

#include <stdlib.h>

typedef double value;

struct value_array {
  size_t capacity;
  size_t count;
  value *values;
};

void value_array_init(struct value_array *);

void value_array_write(struct value_array *, value);

void value_array_free(struct value_array *);

#endif
