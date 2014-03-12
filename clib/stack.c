#include "stack.h"

#include "cmemory.h"

SNode *stack_push(SNode *node, cpointer ptr)
{
	SNode *top = c_new(SNode, 1);
	top->data = ptr;
	top->next = node;
	return top;
}

SNode *stack_pop(SNode *node)
{
	SNode *top = node->next;
	c_delete(node);
	return top;
}

void stack_dispose(SNode *node)
{
	while(node != NULL) {
		node = stack_pop(node);
	}
}
