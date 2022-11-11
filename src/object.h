#ifndef WISP_OBJECT_H
#define WISP_OBJECT_H

#include "chunk.h"
#include "common.h"

enum obj_type {
  OBJ_STRING,
  OBJ_LAMBDA,
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

struct obj_lambda {
  struct obj obj;
  int arity;
  int upvalue_count;
  bool has_param_list;
  struct chunk chunk;
};

struct obj_string *copy_string(const char *, size_t, uint64_t);

struct obj_lambda *new_lambda();

#endif
