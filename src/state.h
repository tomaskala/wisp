#ifndef WISP_STATE_H
#define WISP_STATE_H

#include "table.h"

struct wisp_state {
  // TODO: When strings are implemented, consider whether they should be
  // TODO: interned or not. If not, the implementation of strings, atoms and
  // TODO: the string pool could be greatly simplified.
  // TODO: Advantages: compare strings by identity, in general memory savings
  // TODO: Disadvantages: GC overhead (weak refs), memory overhead if used once
  struct str_pool atoms;
};

void wisp_state_init(struct wisp_state *);

void wisp_state_free(struct wisp_state *);

#endif
