#ifndef STACK_H
#define STACK_H

#include "structs.h"

SNode *stack_push(SNode *node, void *ptr);
SNode *stack_pop(SNode *node);
void stack_dispose(SNode *node);

#endif //STACK_H
