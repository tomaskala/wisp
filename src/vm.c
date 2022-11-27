#include <stdarg.h>
#include <stdio.h>

#include "opcodes.h"
#include "state.h"
#include "table.h"
#include "vm.h"

#ifdef DEBUG_TRACE_EXECUTION
#include "debug.h"
#endif

static void vm_stack_reset(struct wisp_state *w)
{
  w->frame_count = 0;
  w->stack_top = w->stack;
  w->open_upvalues = NULL;
}

static void vm_stack_push(struct wisp_state *w, Value value)
{
  *w->stack_top = value;
  w->stack_top++;
}

static Value vm_stack_pop(struct wisp_state *w)
{
  w->stack_top--;
  return *w->stack_top;
}

static Value vm_stack_peek(struct wisp_state *w, int distance)
{
  return w->stack_top[-1 - distance];
}

static void runtime_error(struct wisp_state *w, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = w->frame_count - 1; i >= 0; --i) {
    struct call_frame *frame = &w->frames[i];
    struct obj_lambda *lambda = frame->closure->lambda;
    size_t instruction = frame->ip - lambda->chunk.code - 1;
    fprintf(stderr, "[line %d]\n", lambda->chunk.lines[instruction]);
    // TODO: Output whether the error is in the script or the current function,
    // TODO: if it has a name.
  }

  vm_stack_reset(w);
}

static struct obj_upvalue *capture_upvalue(struct wisp_state *w, Value *local)
{
  struct obj_upvalue *prev = NULL;
  struct obj_upvalue *upvalue = w->open_upvalues;

  while (upvalue != NULL && upvalue->location > local) {
    prev = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local)
    return upvalue;

  struct obj_upvalue *captured = upvalue_new(w, local);
  captured->next = upvalue;

  if (prev == NULL)
    w->open_upvalues = captured;
  else
    prev->next = captured;

  return captured;
}

static void close_upvalues(struct wisp_state *w, Value *last)
{
  while (w->open_upvalues != NULL && w->open_upvalues->location >= last) {
    struct obj_upvalue *upvalue = w->open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    w->open_upvalues = upvalue->next;
  }
}

static bool call(struct wisp_state *w, struct obj_closure *closure,
    uint8_t arg_count)
{
  if (closure->lambda->has_param_list) {
    // Either (lambda params expr) or (lambda (p1 p2 ... pn . params) expr)
    // Subtract 1 from the lambda's arity which includes the parameter list.
    if (arg_count < closure->lambda->arity - 1) {
      runtime_error(w,
          "Expected at least %" PRIu8 " arguments but got %" PRIu8,
          closure->lambda->arity - 1, arg_count);
      return false;
    }

    // Need to collect all extra arguments in a list and push it as the last
    // argument. Even if no extra arguments have been provided, we need to push
    // an empty list.
    vm_stack_push(w, NIL_VAL);
    uint8_t extra_args = arg_count - (closure->lambda->arity - 1);

    if (extra_args > 0) {
      for (; extra_args > 0; --extra_args) {
        Value cdr = vm_stack_peek(w, 0);
        Value car = vm_stack_peek(w, 1);
        struct obj_pair *pair = pair_new(w, car, cdr);

        vm_stack_pop(w);
        vm_stack_pop(w);
        vm_stack_push(w, OBJ_VAL(pair));
      }
    }
  } else if (arg_count != closure->lambda->arity) {
    runtime_error(w, "Expected %" PRIu8 " arguments but got %" PRIu8,
        closure->lambda->arity, arg_count);
    return false;
  }

  if (w->frame_count == FRAMES_MAX) {
    runtime_error(w, "Stack overflow");
    return false;
  }

  struct call_frame *frame = &w->frames[w->frame_count++];
  frame->closure = closure;
  frame->ip = closure->lambda->chunk.code;
  frame->slots = w->stack_top - arg_count - 1;
  return true;
}

static bool call_value(struct wisp_state *w, Value callee, uint8_t arg_count)
{
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
    case OBJ_CLOSURE:
      return call(w, AS_CLOSURE(callee), arg_count);
    // TODO: Native functions.
    default:
      break;
    }
  }

  runtime_error(w, "Can only call functions");
  return false;
}

static bool vm_run(struct wisp_state *w)
{
  // TODO: Keep the frame ip in a register variable.
  // TODO: Implement optional direct threading.
  struct call_frame *frame = &w->frames[w->frame_count - 1];

  #define READ_BYTE() (*frame->ip++)
  #define READ_CONSTANT() \
    (frame->closure->lambda->chunk.constants.values[READ_BYTE()])
  #define READ_ATOM() AS_ATOM(READ_CONSTANT())

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf(" ");
    for (Value *slot = w->stack; slot < w->stack_top; ++slot) {
      printf("[ ");
      value_print(*slot);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(&frame->closure->lambda->chunk,
        (int) (frame->ip - frame->closure->lambda->chunk.code));
#endif
    uint8_t instruction = READ_BYTE();

    switch (instruction) {
    case OP_CONSTANT: {
      Value constant = READ_CONSTANT();
      vm_stack_push(w, constant);
      break;
    }
    case OP_NIL:
      vm_stack_push(w, NIL_VAL);
      break;
    case OP_CALL: {
      uint8_t arg_count = READ_BYTE();

      if (!call_value(w, vm_stack_peek(w, arg_count), arg_count))
        return false;

      frame = &w->frames[w->frame_count - 1];
      break;
    }
    case OP_DOT_CALL: {
      if (!IS_PAIR(vm_stack_peek(w, 0))) {
        runtime_error(w, "A lambda must be applied to a cons pair");
        return false;
      }

      uint8_t arg_count = READ_BYTE();
      Value cdr = vm_stack_pop(w);

      do {
        struct obj_pair *pair = AS_PAIR(cdr);
        cdr = pair->cdr;
        vm_stack_push(w, pair->car);
        arg_count++;
      } while (IS_PAIR(cdr));

      if (!IS_NIL(cdr)) {
        runtime_error(w, "Attempt to apply a lambda to a non-list pair");
        return false;
      }

      if (!call_value(w, vm_stack_peek(w, arg_count), arg_count))
        return false;

      frame = &w->frames[w->frame_count - 1];
      break;
    }
    case OP_CLOSURE: {
      struct obj_lambda *lambda = AS_LAMBDA(READ_CONSTANT());
      struct obj_closure *closure = closure_new(w, lambda);
      vm_stack_push(w, OBJ_VAL(closure));

      for (int i = 0; i < closure->upvalue_count; ++i) {
        uint8_t is_local = READ_BYTE();
        uint8_t index = READ_BYTE();
        closure->upvalues[i] = is_local
                             ? capture_upvalue(w, frame->slots + index)
                             : frame->closure->upvalues[index];
      }
      break;
    }
    case OP_RETURN: {
      Value result = vm_stack_pop(w);
      close_upvalues(w, frame->slots);
      w->frame_count--;

      if (w->frame_count == 0) {
        vm_stack_pop(w);
        return true;
      }

      w->stack_top = frame->slots;
      vm_stack_push(w, result);
      frame = &w->frames[w->frame_count - 1];
      break;
    }
    case OP_CONS: {
      Value cdr = vm_stack_peek(w, 0);
      Value car = vm_stack_peek(w, 1);
      struct obj_pair *pair = pair_new(w, car, cdr);

      vm_stack_pop(w);
      vm_stack_pop(w);
      vm_stack_push(w, OBJ_VAL(pair));
      break;
    }
    case OP_CAR:
      if (!IS_PAIR(vm_stack_peek(w, 0))) {
        runtime_error(w, "Operand must be a cons pair");
        return false;
      }

      vm_stack_push(w, AS_PAIR(vm_stack_pop(w))->car);
      break;
    case OP_CDR:
      if (!IS_PAIR(vm_stack_peek(w, 0))) {
        runtime_error(w, "Operand must be a cons pair");
        return false;
      }

      vm_stack_push(w, AS_PAIR(vm_stack_pop(w))->cdr);
      break;
    case OP_DEFINE_GLOBAL: {
      struct obj_string *name = READ_ATOM();
      table_set(w, &w->globals, name, vm_stack_peek(w, 0));
      vm_stack_pop(w);
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm_stack_push(w, frame->slots[slot]);
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      vm_stack_push(w, *frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_GET_GLOBAL: {
      struct obj_string *name = READ_ATOM();
      Value val;

      if (!table_get(&w->globals, name, &val)) {
        runtime_error(w, "Undefined variable: '%s'", name->chars);
        return false;
      }

      vm_stack_push(w, val);
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
  vm_stack_reset(w);

  vm_stack_push(w, OBJ_VAL(lambda));
  struct obj_closure *closure = closure_new(w, lambda);
  vm_stack_pop(w);
  vm_stack_push(w, OBJ_VAL(closure));

  call(w, closure, 0);
  bool result = vm_run(w);

  return result;
}
