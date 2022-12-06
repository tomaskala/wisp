#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif

#include "memory.h"
#include "state.h"

#define GC_HEAP_GROW_FACTOR 2

void obj_mark(struct wisp_state *w, struct obj *obj)
{
  if (obj == NULL || obj->is_marked)
    return;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void *) obj);
  obj_print(OBJ_VAL(obj));
  printf("\n");
#endif
  obj->is_marked = true;

  if (w->gray_count >= w->gray_capacity) {
    w->gray_capacity = GROW_CAPACITY(w->gray_capacity);
    w->gray_stack = realloc(w->gray_stack,
        sizeof(struct obj *) * w->gray_capacity);

    // TODO: Pre-allocate a bit of memory and release it here.
    if (w->gray_stack == NULL)
      exit(1);
  }

  w->gray_stack[w->gray_count++] = obj;
}

static void mark_roots(struct wisp_state *w)
{
  for (Value *slot = w->stack; slot < w->stack_top; ++slot)
    if (IS_OBJ(*slot))
      obj_mark(w, AS_OBJ(*slot));

  for (int i = 0; i < w->frame_count; ++i)
    obj_mark(w, (struct obj *) w->frames[i].closure);

  for (struct obj_upvalue *upvalue = w->open_upvalues;
      upvalue != NULL;
      upvalue = upvalue->next)
    obj_mark(w, (struct obj *) upvalue);

  table_mark(w, &w->globals);

  // compiler_mark_roots();  // TODO
}

static void obj_blacken(struct wisp_state *w, struct obj *obj)
{
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void *) obj);
  obj_print(OBJ_VAL(obj));
  printf("\n");
#endif
  switch (obj->type) {
  case OBJ_ATOM:
    break;
  case OBJ_CLOSURE: {
    struct obj_closure *closure = (struct obj_closure *) obj;
    obj_mark(w, (struct obj *) closure->lambda);

    for (int i = 0; i < closure->upvalue_count; ++i)
      obj_mark(w, (struct obj *) closure->upvalues[i]);
    break;
  }
  case OBJ_LAMBDA: {
    struct obj_lambda *lambda = (struct obj_lambda *) obj;

    for (int i = 0; i < lambda->chunk.constants.count; ++i)
      if (IS_OBJ(lambda->chunk.constants.values[i]))
        obj_mark(w, AS_OBJ(lambda->chunk.constants.values[i]));
    break;
  }
  case OBJ_UPVALUE: {
    struct obj_upvalue *upvalue = (struct obj_upvalue *) obj;

    if (IS_OBJ(upvalue->closed))
      obj_mark(w, AS_OBJ(upvalue->closed));
    break;
  }
  case OBJ_PAIR: {
    struct obj_pair *pair = (struct obj_pair *) obj;

    if (IS_OBJ(pair->car))
      obj_mark(w, AS_OBJ(pair->car));

    if (IS_OBJ(pair->cdr))
      obj_mark(w, AS_OBJ(pair->cdr));
    break;
  }
  }
}

static void trace_references(struct wisp_state *w)
{
  while (w->gray_count > 0) {
    struct obj *obj = w->gray_stack[--w->gray_count];
    obj_blacken(w, obj);
  }
}

static void obj_free(struct wisp_state *w, struct obj *obj)
{
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void *) obj, obj->type);
#endif
  switch(obj->type) {
  case OBJ_ATOM: {
    struct obj_string *str = (struct obj_string *) obj;
    FREE_ARRAY(w, char, str->chars, str->len + 1);
    FREE(w, struct obj_string, obj);
    break;
  }
  case OBJ_CLOSURE: {
    struct obj_closure *closure = (struct obj_closure *) obj;
    FREE_ARRAY(w, struct obj_upvalue *, closure->upvalues,
        closure->upvalue_count);
    FREE(w, struct obj_closure, obj);
    break;
  }
  case OBJ_LAMBDA: {
    struct obj_lambda *lambda = (struct obj_lambda *) obj;
    chunk_free(w, &lambda->chunk);
    FREE(w, struct obj_lambda, obj);
    break;
  }
  case OBJ_UPVALUE:
    FREE(w, struct obj_upvalue, obj);
    break;
  case OBJ_PAIR:
    FREE(w, struct obj_pair, obj);
    break;
  }
}

static void sweep(struct wisp_state *w)
{
  struct obj *prev = NULL;
  struct obj *obj = w->objects;

  while (obj != NULL) {
    if (obj->is_marked) {
      obj->is_marked = false;
      prev = obj;
      obj = obj->next;
    } else {
      struct obj *unreached = obj;
      obj = obj->next;

      if (prev == NULL)
        w->objects = obj;
      else
        prev->next = obj;

      obj_free(w, unreached);
    }
  }
}

static void collect_garbage(struct wisp_state *w)
{
#ifdef DEBUG_LOG_GC
  printf("-- gc begin --\n");
  size_t before = w->bytes_allocated;
#endif
  mark_roots(w);
  trace_references(w);
  str_pool_remove_white(w);
  sweep(w);
  w->next_gc = w->bytes_allocated * GC_HEAP_GROW_FACTOR;
#ifdef DEBUG_LOG_GC
  printf("-- gc end --\n");
  printf("  collected %zu bytes (from %zu to %zu), next at %zu\n",
      before - w->bytes_allocated, before, w->bytes_allocated, w->next_gc);
#endif
}

void *wisp_realloc(struct wisp_state *w, void *ptr, size_t old_size,
    size_t new_size)
{
  w->bytes_allocated += new_size - old_size;
  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage(w);
#endif
    if (w->bytes_allocated > w->next_gc)
      collect_garbage(w);
  }

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL)
    exit(1);

  return result;
}

void *wisp_calloc(struct wisp_state *w, size_t old_nmemb, size_t new_nmemb,
    size_t size)
{
  size_t old_size = old_nmemb * size;
  size_t new_size = new_nmemb * size;
  w->bytes_allocated += new_size - old_size;
  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage(w);
#endif
    if (w->bytes_allocated > w->next_gc)
      collect_garbage(w);
  }

  void *result = calloc(new_nmemb, size);
  if (result == NULL)
    exit(1);

  return result;
}

void wisp_free_objs(struct wisp_state *w)
{
  struct obj *obj = w->objects;

  while (obj != NULL) {
    struct obj *next = obj->next;
    obj_free(w, obj);
    obj = next;
  }
}
