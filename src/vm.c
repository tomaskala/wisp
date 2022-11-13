#include <stdarg.h>
#include <stdio.h>

#include "table.h"
#include "vm.h"

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

struct vm {
  // The overall state of the program.
  struct wisp_state *w;

  // Contains all call nested call frames of the current closure execution.
  struct call_frame frames[FRAMES_MAX];

  // Number of currently nested call frames.
  int frame_count;

  // A value must reside on the stack to be marked as reachable.
  Value stack[STACK_MAX];

  // The next value to pop/peek;
  Value *stack_top;
};

static void vm_stack_reset(struct vm *vm)
{
  vm->frame_count = 0;
  vm->stack_top = vm->stack;
  // TODO: Once upvalues are implemented, reset those as well.
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
  (void) vm;
  // TODO
}

static void runtime_error(struct vm *vm, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm->frame_count - 1; i >= 0; --i) {
    struct call_frame *frame = &vm->frames[i];
    struct obj_lambda *lambda = frame->closure->lambda;
    size_t instruction = frame->ip - lambda->chunk.code - 1;
    fprintf(stderr, "[line %d]\n", lambda->chunk.lines[instruction]);
    // TODO: Output whether the error is in the script or the current function,
    // TODO: if it has a name.
  }

  vm_stack_reset(vm);
}

static bool call(struct vm *vm, struct obj_closure *closure, uint8_t arg_count)
{
  if (arg_count != closure->lambda->arity) {
    runtime_error(vm, "Expected %" PRIu8 " arguments but got %" PRIu8,
        closure->lambda->arity, arg_count);
    return false;
  }

  if (vm->frame_count == FRAMES_MAX) {
    runtime_error(vm, "Stack overflow");
    return false;
  }

  struct call_frame *frame = &vm->frames[vm->frame_count++];
  frame->closure = closure;
  frame->ip = closure->lambda->chunk.code;
  frame->slots = vm->stack_top - arg_count - 1;
  return true;
}

static bool vm_run(struct vm *vm)
{
  // TODO: Keep the frame ip in a register variable.
  struct call_frame *frame = &vm->frames[vm->frame_count - 1];

  #define READ_BYTE() (*frame->ip++)
  #define READ_CONSTANT() \
    (frame->closure->lambda->chunk.constants.values[READ_BYTE()])

  for (;;) {
    uint8_t instruction = READ_BYTE();

    switch (instruction) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      vm_stack_push(vm, constant);
      break;
    }
    case OP_NIL:
      vm_stack_push(vm, NIL_VAL);
      break;
    case OP_CALL:
      // TODO
      break;
    case OP_DOT_CALL:
      // TODO
      break;
    case OP_CLOSURE:
      // TODO
      break;
    case OP_RETURN:
      // TODO
      break;
    case OP_CONS: {
      Value b = vm_stack_pop(vm);
      Value a = vm_stack_pop(vm);
      vm_stack_push(vm, OBJ_VAL(new_pair(&a, &b)));
      break;
    }
    case OP_CAR:
      if (!IS_PAIR(vm_stack_peek(vm))) {
        runtime_error(vm, "Operand must be a cons pair");
        return false;
      }
      vm_stack_push(vm, OBJ_VAL(AS_PAIR(vm_stack_pop(vm))->car));
      break;
    case OP_CDR:
      if (!IS_PAIR(vm_stack_peek(vm))) {
        runtime_error(vm, "Operand must be a cons pair");
        return false;
      }
      vm_stack_push(vm, OBJ_VAL(AS_PAIR(vm_stack_pop(vm))->cdr));
      break;
    case OP_DEFINE_GLOBAL:
      // TODO
      break;
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm_stack_push(vm, frame->slots[slot]);
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      vm_stack_push(vm, *frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_GET_GLOBAL:
      // TODO
      break;
    }
  }

  #undef READ_CONSTANT
  #undef READ_BYTE
}

bool interpret(struct wisp_state *w, struct obj_lambda *lambda)
{
  struct vm vm;
  vm_init(&vm, w);

  vm_stack_push(&vm, OBJ_VAL(lambda));
  struct obj_closure *closure = new_closure(lambda);
  vm_stack_pop(&vm);
  vm_stack_push(&vm, OBJ_VAL(closure));

  call(&vm, closure, 0);
  bool result = vm_run(&vm);

  vm_free(&vm);
  return result;
}
