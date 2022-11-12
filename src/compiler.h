#ifndef WISP_COMPILER_H
#define WISP_COMPILER_H

#include "object.h"
#include "state.h"

struct obj_lambda *compile(struct wisp_state *, const char *);

#endif
