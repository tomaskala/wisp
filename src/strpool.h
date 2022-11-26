#ifndef WISP_STRPOOL_H
#define WISP_STRPOOL_H

// Source: https://nullprogram.com/blog/2022/08/08/

#include "common.h"
#include "state.h"

void str_pool_init(struct wisp_state *);

struct obj_string *str_pool_intern(struct wisp_state *, const char *, size_t);

void str_pool_free(struct wisp_state *);

#endif
