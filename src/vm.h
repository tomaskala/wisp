#ifndef WISP_VM_H
#define WISP_VM_H

#include "common.h"
#include "object.h"
#include "state.h"

bool interpret(struct wisp_state *, struct obj_lambda *);

#endif
