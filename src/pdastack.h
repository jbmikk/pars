#ifndef PDASTACK_H
#define PDASTACK_H

#include "stack.h"
#include "fsm.h"
#include "token.h"

#define PDA_NODE_MODE 0
#define PDA_NODE_BACKTRACK 1
#define PDA_NODE_SR 2
#define PDA_NODE_SA 3
#define PDA_NODE_REDUCTION 4

typedef struct PDANode {
	char type;
	// TODO: Maybe in the future we can use only symbols.
	// We should map the states to symbols.
	State *state;
	Token token;
	char path;
} PDANode;

DEFINE(Stack, PDANode, PDANode, pdanode);

typedef struct PDAStack {
	StackPDANode stack;
	State *start;
} PDAStack;

void pdastack_init(PDAStack *pdastack);
void pdastack_dispose(PDAStack *pdastack);
void pdastack_start(PDAStack *pdastack, State *state);
bool pdastack_is_empty(PDAStack *pdastack);
bool pdastack_has_reduction(PDAStack *pdastack);
PDANode pdastack_top(PDAStack *pdastack);
void pdastack_pop(PDAStack *pdastack);
State *pdastack_get_start(PDAStack *pdastack);
void pdastack_state_push(PDAStack *pdastack, PDANode tnode);
PDANode pdastack_state_pop(PDAStack *pdastack, char type, bool *is_empty);
void pdastack_mode_push(PDAStack *pdastack, State *mode_state);
State *pdastack_mode_pop(PDAStack *pdastack);

#endif //PDASTACK_H
