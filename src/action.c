#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include <stdio.h>

//# State functions

void state_init(State *state)
{
	radix_tree_init(&state->actions, 0, 0, NULL);
}

//# Action functions

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		T1? ((Action*)T1)->state: NULL, \
		T2, \
		T2? ((Action*)T2)->state: NULL, \
		A, \
		(R != 0)? '>': ' ', \
		R, \
		S, (char)S \
	)
#else
#define trace(M, T1, T2, S, A, R)
#endif

void action_init(Action *action, char type, int reduction, State *state)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
}

Action *action_add_buffer(Action *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	if(action == NULL) {
		action = c_new(Action, 1);
		action_init(action, type, reduction, NULL);
	}

	if(!from->state) {
		from->state = c_new(State, 1);
		state_init(from->state);
	}
	Action *prev = (Action *)radix_tree_try_set(&from->state->actions, buffer, size, action);
	if(prev) {
		if(
			prev->type == action->type &&
			prev->reduction == action->reduction
		) {
			trace("dup", from, action, array_to_int(buffer, size), "skip", reduction);
		} else {
			trace("dup", from, action, array_to_int(buffer, size), "conflict", reduction);
			//TODO: add sentinel ?
		}
		c_delete(action);
		action = NULL;
	}
	return action;
}

Action *action_add(Action *from, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	action_init(action, type, reduction, NULL);

	if(!from->state) {
		from->state = c_new(State, 1);
		state_init(from->state);
	}
	//TODO: detect duplicates
	radix_tree_set_int(&from->state->actions, symbol, action);

	if(type == ACTION_TYPE_CONTEXT_SHIFT) {
		trace("add", from, action, symbol, "context-shift", 0);
	} else if(type == ACTION_TYPE_SHIFT) {
		trace("add", from, action, symbol, "shift", 0);
	} else if(type == ACTION_TYPE_REDUCE) {
		trace("add", from, action, symbol, "reduce", 0);
	} else if(type == ACTION_TYPE_ACCEPT) {
		trace("add", from, action, symbol, "accept", 0);
	} else {
		trace("add", from, action, symbol, "action", 0);
	}
	return action;
}

void action_add_first_set(Action *from, State* state)
{
	Action *ac;
	Iterator it;
	radix_tree_iterator_init(&it, &(state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		action_add_buffer(from, it.key, it.size, 0, 0, ac);
		trace("add", from, ac, array_to_int(it.key, it.size), "first", 0);
	}
	radix_tree_iterator_dispose(&it);
}

void action_add_reduce_follow_set(Action *from, Action *to, int symbol)
{
	Action *ac;
	Iterator it;

	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.
	radix_tree_iterator_init(&it, &(to->state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		action_add_buffer(from, it.key, it.size, ACTION_TYPE_REDUCE, symbol, NULL);
		trace("add", from, ac, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}

