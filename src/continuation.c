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
	// Should events insert symbols?
	// Possibly, then a specific action with reduction  can be matched.
	// But we don't really want to accept or reduce anything. We really
	// need a different kind of action. Do we? Maybe it's just a drop.
	// A drop action looping back to the same state (avoid infinite loop!)
	// Actually, it should be a kind of non-popping reduction.
	// A non-popping reduction would not really be a reduction. It's more
	// It would just match a generic partial symbol, then push a symbol
	// on top of the input (without popping anything), then match it (drop
	// it, this is the match we care about, like an accept) then continue
	// matching as if nothing had happened.
	// Basically we need two things, a kind of reduction that does't pop
	// anything and an accept whose continuation is the same.
	// For this to happen we need to trigger the reduction somehow.
	// If we let the regular matching process to move forward the reduction
	// won't happen. In order to trigger it we need a pseudo-symbol.
	// Something like a "leaving state" or "state reached" pseudo-symbol
	// In case of reduce-reduce conflict we should somehow match both or
	// merge the rules into one.
	// Pushing symbols here is dangerous, because we going to lose state
	// from the current transition. We no longer know what we just matched
	// Whatever it was it would have to be retried later again to properly
	// continue. It is not unlike reductions and their lookahead.
	// In fact, lookahead seems to be what we need here. We need to match
	// the next symbol without consuming it, so we can match it again 
	// later. Lookahead right now is supported by using CONTINUATION_PUSH.
	// When we push we actually do a double push, first we push the 
	// currently matched symbol, and then a new one. After matching the
	// new one, then we match again the original. This is ok for 
	// reductions because after matching the new symbol the machine is in
	// a different state, so matching the previously matched symbol does
	// not yield the same result again.
	// But in the case of partial matching we may end up in an infinite
	// loop.
	if(t.from->events & STATE_EVENT_OUT) {
		// CONTINUATION PUSH (OUT SYMBOL)?
		// CONTINUATION_PUSH does not exist anymore, we need to push
		// into the stack like a reduction, but without popping.
	} else if(t.to->events & STATE_EVENT_IN) {
		// CONTINUATION PUSH (IN SYMBOL)?
	}


	return cont;
}
