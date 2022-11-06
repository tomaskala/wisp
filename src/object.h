#ifndef WISP_OBJECT_H
#define WISP_OBJECT_H

#include "chunk.h"
#include "common.h"

enum obj_type {
  OBJ_LAMBDA,
};

struct obj {
  enum obj_type type;
};

struct obj_lambda {
  struct obj obj;
  int arity;
  int upvalue_count;
  bool has_param_list;
  struct chunk chunk;
};

struct obj_lambda *new_lambda();

#endif
