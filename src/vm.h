#ifndef WISP_VM_H
#define WISP_VM_H

#include "common.h"
#include "memory.h"
#include "value.h"

void vm_stack_reset(struct wisp_state *);

bool interpret(struct wisp_state *, struct obj_lambda *);

#endif
