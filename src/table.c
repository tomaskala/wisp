#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

void table_init(struct table *table)
{
  table->capacity = 0;
  table->count = 0;
  table->ht = NULL;
}

static struct table_node *find_entry(struct table_node *ht, int capacity,
    struct obj_string *key)
{
  uint32_t index = key->hash & (capacity - 1);
  struct table_node *tombstone = NULL;

  for (;;) {
    struct table_node *node = &ht[index];

    if (node->key == NULL) {
      if (IS_NIL(node->val))
        return tombstone != NULL ? tombstone : node;
      else if (tombstone == NULL)
        tombstone = node;
    } else if (node->key == key)
      return node;

    index = (index + 1) & (capacity - 1);
  }
}

static void adjust_capacity(struct wisp_state *w, struct table *table,
    int capacity)
{
  struct table_node *ht = ALLOCATE(w, struct table_node, capacity);

  for (int i = 0; i < capacity; ++i) {
    ht[i].key = NULL;
    ht[i].val = NIL_VAL;
  }

  table->count = 0;

  for (int i = 0; i < table->capacity; ++i) {
    struct table_node *node = &table->ht[i];

    if (node->key == NULL)
      continue;

    struct table_node *dest = find_entry(ht, capacity, node->key);
    dest->key = node->key;
    dest->val = node->val;
    table->count++;
  }

  FREE_ARRAY(w, struct table_node, table->ht, table->capacity);
  table->ht = ht;
  table->capacity = capacity;
}

bool table_get(struct table *table, struct obj_string *key, Value *val)
{
  if (table->count == 0)
    return false;

  struct table_node *node = find_entry(table->ht, table->capacity, key);

  if (node->key == NULL)
    return false;

  *val = node->val;
  return true;
}

bool table_set(struct wisp_state *w, struct table *table,
    struct obj_string *key, Value val)
{
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(w, table, capacity);
  }

  struct table_node *node = find_entry(table->ht, table->capacity, key);
  bool is_new_key = node->key == NULL;

  if (is_new_key && IS_NIL(node->val))
    table->count++;

  node->key = key;
  node->val = val;
  return is_new_key;
}

bool table_delete(struct table *table, struct obj_string *key)
{
  if (table->count == 0)
    return false;

  struct table_node *node = find_entry(table->ht, table->capacity, key);

  if (node->key == NULL)
    return false;

  node->key = NULL;
  // TODO: Replace by BOOL_VAL(true) once booleans are implemented.
  node->val = NUM_VAL(1);
  return true;
}

void table_free(struct wisp_state *w, struct table *table)
{
  FREE_ARRAY(w, struct table_node, table->ht, table->capacity);
  table_init(table);
}
