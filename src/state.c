#include "state.h"

void wisp_state_init(struct wisp_state *w)
{
  str_pool_init(&w->str_pool);
}

void wisp_state_free(struct wisp_state *w)
{
  str_pool_free(&w->str_pool);
  wisp_state_init(w);
}
