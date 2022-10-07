#include "memory.h"
#include "value.h"

void value_array_init(struct value_array *array)
{
  array->capacity = 0;
  array->count = 0;
  array->values = NULL;
}

void value_array_write(struct value_array *array, value val)
{
  if (array->count >= array->capacity) {
    array->capacity = GROW_CAPACITY(array->capacity);
    array->values = GROW_ARRAY(value, array->values, array->capacity);
  }

  array->values[array->count] = val;
  array->count++;
}

void value_array_free(struct value_array *array)
{
  FREE(array->values);
  value_array_init(array);
}
