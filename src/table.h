#ifndef WISP_TABLE_H
#define WISP_TABLE_H

#include "common.h"
#include "value.h"

struct table_node {
  struct obj_string *key;
  Value val;
};

// TODO: Unify the logic with string pool?
struct table {
  int capacity;
  int count;
  struct table_node *ht;
};

void table_init(struct table *);

bool table_get(struct table *, struct obj_string *, Value *);

bool table_set(struct wisp_state *, struct table *, struct obj_string *,
    Value);

bool table_delete(struct table *, struct obj_string *);

void table_free(struct wisp_state *, struct table *);

#endif
