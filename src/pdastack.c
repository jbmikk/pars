#include "pdastack.h"

#include <stdlib.h>
#include <stdio.h>

FUNCTIONS(Stack, PDANode, PDANode, pdanode);

void pdastack_init(PDAStack *pdastack)
{
	pdastack->start = NULL;
	stack_pdanode_init(&pdastack->stack);
}

void pdastack_dispose(PDAStack *pdastack)
{
	stack_pdanode_dispose(&pdastack->stack);
}

void pdastack_start(PDAStack *pdastack, State *state)
{
	pdastack->start = state;	
}

State *pdastack_get_start(PDAStack *pdastack)
{
	return pdastack->start;
}

bool pdastack_is_empty(PDAStack *pdastack)
{
	bool is_empty = stack_pdanode_is_empty(&pdastack->stack);
	bool is_initial = false;
	if(!is_empty) {
		is_initial = pdastack->stack.stack.top->next == NULL;
	}
	return is_empty || is_initial;
}

void pdastack_state_push(PDAStack *pdastack, PDANode tnode)
{
	stack_pdanode_push(&pdastack->stack, tnode);
}

PDANode pdastack_state_pop(PDAStack *pdastack, char type, bool *is_empty)
{
	PDANode top;

	// TODO: replace is_empty with Maybe(PDANode) generic.
	while (!(*is_empty = stack_pdanode_is_empty(&pdastack->stack))) {
		top = stack_pdanode_top(&pdastack->stack);
		stack_pdanode_pop(&pdastack->stack);
		if(top.type == type) {
			break;
		}
	}
	return top;
}

void pdastack_mode_push(PDAStack *pdastack, State *mode_state)
{
	pdastack_state_push(pdastack, (PDANode){ 
		PDA_NODE_MODE,
		pdastack->start,
		0
	});
	pdastack->start = mode_state;
}

State *pdastack_mode_pop(PDAStack *pdastack)
{
	bool is_empty;
	PDANode popped = pdastack_state_pop(pdastack, PDA_NODE_MODE, &is_empty);
	if(is_empty) {
		//sentinel("Mode pop fail");
		printf("FTH %p: mode pop fail\n", pdastack);
		exit(1);
	}
	pdastack->start = popped.state;
	return pdastack->start;
}

