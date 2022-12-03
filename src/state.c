#include "memory.h"
#include "state.h"
#include "vm.h"

void wisp_state_init(struct wisp_state *w)
{
  vm_stack_reset(w);
  w->objects = NULL;
  str_pool_init(w);
  table_init(&w->globals);
  w->bytes_allocated = 0;
  w->next_gc = 1024 * 1024;
  w->gray_count = 0;
  w->gray_capacity = 0;
  w->gray_stack = NULL;
}

void wisp_state_free(struct wisp_state *w)
{
  wisp_free_objects(w);
  free(w->gray_stack);
  table_free(w, &w->globals);
  str_pool_free(w);
  wisp_state_init(w);
  vm_stack_reset(w);
}
