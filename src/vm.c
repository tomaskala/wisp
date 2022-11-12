#include "vm.h"

#define STACK_MAX UINT8_COUNT

struct vm {
  // The overall state of the program.
  struct wisp_state *w;

  // A value must reside on the stack to be marked as reachable.
  Value stack[STACK_MAX];

  // The next value to pop/peek;
  Value *stack_top;
};

static void vm_stack_reset(struct vm *vm)
{
  vm->stack_top = vm->stack;
  // TODO: Once frames are implemented, reset those as well.
}

static void vm_init(struct vm *vm, struct wisp_state *w)
{
  vm->w = w;
  vm_stack_reset(vm);
}

static void vm_stack_push(struct vm *vm, Value value)
{
  *vm->stack_top = value;
  vm->stack_top++;
}

static Value vm_stack_pop(struct vm *vm)
{
  vm->stack_top--;
  return *vm->stack_top;
}

static Value vm_stack_peek(struct vm *vm)
{
  return vm->stack_top[-1];
}

static void vm_free(struct vm *vm)
{
  // TODO
}

bool interpret(struct wisp_state *w, struct obj_lambda *lambda)
{
  struct vm vm;
  vm_init(&vm, w);

  vm_stack_push(&vm, OBJ_VAL(lambda));

  vm_free(&vm);
  return false;
}
