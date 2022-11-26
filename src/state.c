#include "state.h"
#include "strpool.h"
#include "value.h"

void wisp_state_init(struct wisp_state *w)
{
  str_pool_init(w);
  table_init(&w->globals);
}

void wisp_state_free(struct wisp_state *w)
{
  table_free(&w->globals);
  str_pool_free(w);
  wisp_state_init(w);
}
