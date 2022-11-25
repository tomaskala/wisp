#ifndef WISP_STRPOOL_H
#define WISP_STRPOOL_H

// Source: https://nullprogram.com/blog/2022/08/08/

#include "common.h"
#include "value.h"

struct str_pool {
  // Exact type of strings being interned.
  enum obj_type str_type;

  // log2(capacity)
  int exp;

  // Number of elements present in the hash table.
  int count;

  // Array representation of the hash table.
  struct obj_string **ht;
};

void str_pool_init(struct str_pool *, enum obj_type);

struct obj_string *str_pool_intern(struct str_pool *, const char *, size_t);

void str_pool_free(struct str_pool *);

#endif
