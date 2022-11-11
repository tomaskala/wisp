#include "object.h"
#include "state.h"

void wisp_state_init(struct wisp_state *w)
{
  str_pool_init(&w->atoms, OBJ_ATOM);
}

void wisp_state_free(struct wisp_state *w)
{
  str_pool_free(&w->atoms);
  wisp_state_init(w);
}
