#ifndef WISP_STATE_H
#define WISP_STATE_H

#include "table.h"

struct wisp_state {
  struct str_pool str_pool;
};

void wisp_state_init(struct wisp_state *);

void wisp_state_free(struct wisp_state *);

#endif
