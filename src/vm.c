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

  // List of upvalues pointing to local variables still on the stack.
  struct obj_upvalue *open_upvalues;
};

static void vm_stack_reset(struct vm *vm)
{
  vm->frame_count = 0;
  vm->stack_top = vm->stack;
  vm->open_upvalues = NULL;
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

static Value vm_stack_peek(struct vm *vm, int distance)
{
  return vm->stack_top[-1 - distance];
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

static struct obj_upvalue *capture_upvalue(struct vm *vm, Value *local)
{
  struct obj_upvalue *prev = NULL;
  struct obj_upvalue *upvalue = vm->open_upvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prev = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local)
    return upvalue;

  struct obj_upvalue *captured = new_upvalue(local);
  captured->next = upvalue;

  if (prev == NULL)
    vm->open_upvalues = captured;
  else
    prev->next = captured;

  return captured;
}

static void close_upvalues(struct vm *vm, Value *last)
{
  while (vm->open_upvalues != NULL && vm->open_upvalues->location >= last) {
    struct obj_upvalue *upvalue = vm->open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm->open_upvalues = upvalue->next;
  }
}

static bool call(struct vm *vm, struct obj_closure *closure, uint8_t arg_count)
{
  if (closure->lambda->has_param_list) {
    // Either (lambda params expr) or (lambda (p1 p2 ... pn . params) expr)
    // Subtract 1 from the lambda's arity which includes the parameter list.
    if (arg_count < closure->lambda->arity - 1) {
      runtime_error(vm,
          "Expected at least %" PRIu8 " arguments but got %" PRIu8,
          closure->lambda->arity - 1, arg_count);
      return false;
    }

    // Need to collect all extra arguments in a list and push it as the last
    // argument. Even if no extra arguments have been provided, we need to push
    // an empty list.
    vm_stack_push(vm, NIL_VAL);
    uint8_t extra_args = arg_count - (closure->lambda->arity - 1);

    if (extra_args > 0) {
      for (; extra_args > 0; --extra_args) {
        Value b = vm_stack_peek(vm, 0);
        Value a = vm_stack_peek(vm, 1);

        int car = vm->w->cells.count;
        value_array_write(&vm->w->cells, a);
        value_array_write(&vm->w->cells, b);

        vm_stack_pop(vm);
        vm_stack_pop(vm);
        vm_stack_push(vm, CONS_VAL(car));
      }
    }
  } else if (arg_count != closure->lambda->arity) {
    runtime_error(vm,
        "Expected %" PRIu8 " arguments but got %" PRIu8,
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

static bool call_value(struct vm *vm, Value callee, uint8_t arg_count)
{
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_CLOSURE:
      return call(vm, AS_CLOSURE(callee), arg_count);
    // TODO: Native functions.
    default:
      break;
    }
  }

  runtime_error(vm, "Can only call functions");
  return false;
}

static bool vm_run(struct vm *vm)
{
  // TODO: Keep the frame ip in a register variable.
  // TODO: Implement optional direct threading.
  struct call_frame *frame = &vm->frames[vm->frame_count - 1];

  #define READ_BYTE() (*frame->ip++)
  #define READ_CONSTANT() \
    (frame->closure->lambda->chunk.constants.values[READ_BYTE()])
  #define READ_ATOM() AS_ATOM(READ_CONSTANT())

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
    case OP_CALL: {
      uint8_t arg_count = READ_BYTE();

      if (!call_value(vm, vm_stack_peek(vm, arg_count), arg_count))
        return false;

      frame = &vm->frames[vm->frame_count - 1];
      break;
    }
    case OP_DOT_CALL: {
      if (!IS_CONS(vm_stack_peek(vm, 0))) {
        runtime_error(vm, "A lambda must be applied to a cons pair");
        return false;
      }

      uint8_t arg_count = READ_BYTE();
      Value cons = vm_stack_pop(vm);

      do {
        Value car = vm->w->cells.values[AS_CAR(cons)];
        Value cdr = vm->w->cells.values[AS_CDR(cons)];
        vm_stack_push(vm, car);
        arg_count++;
        cons = cdr;
      } while (IS_CONS(cons));

      if (!IS_NIL(cons)) {
        runtime_error(vm, "Attempt to apply a lambda to a non-list pair");
        return false;
      }

      if (!call_value(vm, vm_stack_peek(vm, arg_count), arg_count))
        return false;

      frame = &vm->frames[vm->frame_count - 1];
      break;
    }
    case OP_CLOSURE: {
      struct obj_lambda *lambda = AS_LAMBDA(READ_CONSTANT());
      struct obj_closure *closure = new_closure(lambda);
      vm_stack_push(vm, OBJ_VAL(closure));

      for (int i = 0; i < closure->upvalue_count; ++i) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        closure->upvalues[i] = is_local
                             ? capture_upvalue(vm, frame->slots + index)
                             : frame->closure->upvalues[index];
      }
      break;
    }
    case OP_RETURN: {
      Value result = vm_stack_pop(vm);
      close_upvalues(vm, frame->slots);
      vm->frame_count--;

      if (vm->frame_count == 0) {
        vm_stack_pop(vm);
        return true;
      }

      vm->stack_top = frame->slots;
      vm_stack_push(vm, result);
      frame = &vm->frames[vm->frame_count - 1];
      break;
    }
    case OP_CONS: {
      Value b = vm_stack_peek(vm, 0);
      Value a = vm_stack_peek(vm, 1);

      int car = vm->w->cells.count;
      value_array_write(&vm->w->cells, a);
      value_array_write(&vm->w->cells, b);

      vm_stack_pop(vm);
      vm_stack_pop(vm);
      vm_stack_push(vm, CONS_VAL(car));
      break;
    }
    case OP_CAR:
      if (!IS_CONS(vm_stack_peek(vm, 0))) {
        runtime_error(vm, "Operand must be a cons pair");
        return false;
      }

      vm_stack_push(vm, vm->w->cells.values[AS_CAR(vm_stack_pop(vm))]);
      break;
    case OP_CDR:
      if (!IS_CONS(vm_stack_peek(vm, 0))) {
        runtime_error(vm, "Operand must be a cons pair");
        return false;
      }

      vm_stack_push(vm, vm->w->cells.values[AS_CDR(vm_stack_pop(vm))]);
      break;
    case OP_DEFINE_GLOBAL: {
      struct obj_string *name = READ_ATOM();
      table_set(&vm->w->globals, name, vm_stack_peek(vm, 0));
      vm_stack_pop(vm);
      break;
    }
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
    case OP_GET_GLOBAL: {
      struct obj_string *name = READ_ATOM();
      Value val;

      if (!table_get(&vm->w->globals, name, &val)) {
        runtime_error(vm, "Undefined variable: '%s'", name->chars);
        return false;
      }

      vm_stack_push(vm, val);
      break;
    }
    }
  }

  #undef READ_ATOM
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
