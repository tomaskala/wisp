#ifndef WISP_COMPILER_H
#define WISP_COMPILER_H

#include "state.h"
#include "value.h"

struct obj_lambda *compile(struct wisp_state *, const char *);

#endif
