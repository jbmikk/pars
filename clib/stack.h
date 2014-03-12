#ifndef STACK_H
#define STACK_H

#include "ctypes.h"
#include "cstruct.h"

SNode *stack_push(SNode *node, cpointer ptr);
SNode *stack_pop(SNode *node);
void stack_dispose(SNode *node);

#endif //STACK_H
