#ifndef TRANSITION_H
#define TRANSITION_H

#include "fsm.h"
#include "token.h"

typedef struct Transition {
	State *from;
	State *to;
	Action *action;
	char path;
	Token token;
	// Maybe reduction and popped should be a single stack variable
	Token reduction;
	Token popped;
	// TODO: If actions weren't pointers we could build a BT action.
	char backtrack;
} Transition;

#endif //TRANSITION_H
