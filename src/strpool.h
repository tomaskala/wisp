#ifndef WISP_STRPOOL_H
#define WISP_STRPOOL_H

// Source: https://nullprogram.com/blog/2022/08/08/

#include "common.h"

struct str_pool {
  // log2(capacity)
  int exp;

  // Number of elements present in the hash table.
  int count;

  // Array representation of the hash table.
  struct obj_string **ht;
};

void str_pool_init(struct wisp_state *);

struct obj_string *str_pool_intern(struct wisp_state *, const char *, size_t);

void str_pool_remove_white(struct wisp_state *);

void str_pool_free(struct wisp_state *);

#endif
