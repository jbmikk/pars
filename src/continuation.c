#include "continuation.h"

Continuation continuation_build(Transition t, int error)
{

	Continuation cont;
	cont.token = t.token;

	if(error) {
		cont.type = CONTINUATION_ERROR; 
	} else {
		// TODO: maybe actions can have an input operation, something
		// like: Consume, Lookahead(do nothing), something else?
		switch(t.action->type) {
		case ACTION_START:
		case ACTION_SHIFT:
		case ACTION_DROP:
			cont.type = CONTINUATION_NEXT; 
			break;
		case ACTION_ACCEPT:
			if(t.action->reduction == NONE) {
				cont.type = CONTINUATION_NEXT;
			} else {
				cont.type = CONTINUATION_RETRY;
			}
			break;
		case ACTION_EMPTY:
		case ACTION_REDUCE:
		case ACTION_POP:
		case ACTION_POP_SHIFT:
			cont.type = CONTINUATION_RETRY; 
			break;
		case ACTION_ERROR:
			if(t.backtrack > 0) {
				cont.type = CONTINUATION_RETRY;
			} else {
				cont.type = CONTINUATION_ERROR; 
			}
			break;
		default:
			// sentinel?
			break;
		}
	}

	return cont;
}
