#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void value_array_init(struct value_array *array)
{
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void value_array_write(struct value_array *array, Value val)
{
  if (array->count >= array->capacity) {
    array->capacity = GROW_CAPACITY(array->capacity);
    array->values = GROW_ARRAY(Value, array->values, array->capacity);
  }

  array->values[array->count] = val;
  array->count++;
}

void value_array_free(struct value_array *array)
{
  FREE(array->values);
  value_array_init(array);
}

void value_print(Value val)
{
  switch (val.type) {
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUM:
    printf("%g", AS_NUM(val));
    break;
  case VAL_CONS:
    printf("cons");  // TODO: Finish once VList is implemented.
    break;
  case VAL_OBJ:
    object_print(val);
    break;
  }
}
