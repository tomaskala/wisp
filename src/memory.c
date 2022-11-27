#ifdef DEBUG_LOG_GC
#include <stdio.h>
#endif

#include "memory.h"
#include "state.h"
#include "value.h"

static void wisp_collect_garbage(struct wisp_state *w)
{
  (void) w;
  // TODO
}

void *wisp_realloc(struct wisp_state *w, void *ptr, size_t old_size,
    size_t new_size)
{
  w->bytes_allocated += new_size - old_size;
  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    wisp_collect_garbage(w);
#endif
    if (w->bytes_allocated > w->next_gc)
      wisp_collect_garbage(w);
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
    wisp_collect_garbage(w);
#endif
    if (w->bytes_allocated > w->next_gc)
      wisp_collect_garbage(w);
  }

  void *result = calloc(new_nmemb, size);
  if (result == NULL)
    exit(1);

  return result;
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

void wisp_free_objects(struct wisp_state *w)
{
  struct obj *obj = w->objects;

  while (obj != NULL) {
    struct obj *next = obj->next;
    obj_free(w, obj);
    obj = next;
  }
}
