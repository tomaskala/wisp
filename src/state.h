#ifndef WISP_STATE_H
#define WISP_STATE_H

#include "common.h"
#include "strpool.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

struct call_frame {
  // Currently executed closure.
  struct obj_closure *closure;

  // The next instruction to be executed.
  uint8_t *ip;

  // The first slot in the VM value stack the closure can use.
  Value *slots;
};

struct wisp_state {
  // Contains all call nested call frames of the current closure execution.
  struct call_frame frames[FRAMES_MAX];

  // Number of currently nested call frames.
  int frame_count;

  // A value must reside on the stack to be marked as reachable.
  Value stack[STACK_MAX];

  // The next value to pop/peek;
  Value *stack_top;

  // List of upvalues pointing to local variables still on the stack.
  struct obj_upvalue *open_upvalues;

  // List of all collectable objects.
  struct obj *objects;

  // TODO: When strings are implemented, consider whether they should be
  // TODO: interned or not. If not, the implementation of strings, atoms and
  // TODO: the string pool could be greatly simplified.
  // TODO: Advantages: compare strings by identity, in general memory savings
  // TODO: Disadvantages: GC overhead (weak refs), memory overhead if used once

  // Hash table for strings.
  struct str_pool str_pool;

  // A mapping of global variable names to their values.
  struct table globals;

  // Total number of allocated bytes.
  size_t bytes_allocated;

  // Once 'bytes_allocated' exceeds this number, do a GC run.
  size_t next_gc;

  // Number of gray objects currently in 'gray_stack'.
  int gray_count;

  // Capacity of the 'gray_stack' array.
  int gray_capacity;

  // Contains all collectable objects marked gray in the current GC run.
  struct obj **gray_stack;
};

void wisp_state_init(struct wisp_state *);

void wisp_state_free(struct wisp_state *);

#endif
