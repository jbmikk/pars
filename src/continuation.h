#ifndef FSM_CONTINUATION_H
#define FSM_CONTINUATION_H

#include "token.h"
#include "transition.h"

#define CONTINUATION_NEXT 0
#define CONTINUATION_PUSH 1
#define CONTINUATION_RETRY 2
#define CONTINUATION_START 3
#define CONTINUATION_ERROR 4

typedef struct Continuation {
	char type;
	Token token;
	Token token2;
} Continuation;

Continuation continuation_build(Transition t, int error);

#endif //FSM_CONTINUATION_H
