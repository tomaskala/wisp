#ifndef WISP_TABLE_H
#define WISP_TABLE_H

// Source: https://nullprogram.com/blog/2022/08/08/

struct str_pool {
  // log2(capacity)
  int exp;

  // Number of elements present in the hash table.
  int count;

  // Array representation of the hash table.
  struct obj_string **ht;
};

void str_pool_init(struct str_pool *);

struct obj_string *str_pool_intern(struct str_pool *, const char *, size_t);

void str_pool_free(struct str_pool *);

#endif
