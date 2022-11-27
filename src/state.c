#include "memory.h"
#include "state.h"

void wisp_state_init(struct wisp_state *w)
{
  w->objects = NULL;
  str_pool_init(w);
  table_init(&w->globals);
}

void wisp_state_free(struct wisp_state *w)
{
  wisp_free_objects(w);
  table_free(w, &w->globals);
  str_pool_free(w);
  wisp_state_init(w);
}
