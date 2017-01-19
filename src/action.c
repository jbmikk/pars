#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"
#include <stdio.h>

//# State functions

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		T1, \
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

void state_init(State *state)
{
	radix_tree_init(&state->actions, 0, 0, NULL);
	radix_tree_init(&state->refs, 0, 0, NULL);
	state->status = STATE_CLEAR;
}

void state_add_reference(State *state, Symbol *symbol, State *to_state)
{
	Reference *ref = c_new(Reference, 1);
	ref->state = state;
	ref->to_state = to_state;
	//TODO: Is it really necessary? Not used right now.
	ref->symbol = symbol;
	ref->status = REF_PENDING;

	//Is ref key ok?
	radix_tree_set_intptr(&state->refs, (intptr_t)ref, ref);
	state->status |= STATE_INVOKE_REF;
}

void state_dispose(State *state)
{
	Iterator it;

	//Delete all actions
	Action *ac;
	radix_tree_iterator_init(&it, &state->actions);
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		c_delete(ac);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&state->actions);

	//Delete all references
	Reference *ref;
	radix_tree_iterator_init(&it, &state->refs);
	while(ref = (Reference *)radix_tree_iterator_next(&it)) {
		c_delete(ref);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&state->refs);
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	action_init(action, type, reduction, NULL);

	//TODO: detect duplicates
	radix_tree_set_int(&state->actions, symbol, action);

	if(type == ACTION_CONTEXT_SHIFT) {
		trace("add", state, action, symbol, "context-shift", 0);
	} else if(type == ACTION_SHIFT) {
		trace("add", state, action, symbol, "shift", 0);
	} else if(type == ACTION_REDUCE) {
		trace("add", state, action, symbol, "reduce", 0);
	} else if(type == ACTION_ACCEPT) {
		trace("add", state, action, symbol, "accept", 0);
	} else {
		trace("add", state, action, symbol, "action", 0);
	}
	return action;
}

static Action *_state_add_buffer(State *state, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	if(action == NULL) {
		action = c_new(Action, 1);
		action_init(action, type, reduction, NULL);
	}

	Action *prev = (Action *)radix_tree_try_set(&state->actions, buffer, size, action);
	if(prev) {
		if(
			prev->type == action->type &&
			prev->reduction == action->reduction
		) {
			trace("dup", state, action, array_to_int(buffer, size), "skip", reduction);
		} else {
			trace("dup", state, action, array_to_int(buffer, size), "conflict", reduction);
			//TODO: add sentinel ?
		}
		c_delete(action);
		action = NULL;
	}
	return action;
}

void state_add_first_set(State *state, State* source)
{
	Action *action, *clone;
	Iterator it;
	radix_tree_iterator_init(&it, &(source->actions));
	while(action = (Action *)radix_tree_iterator_next(&it)) {
		//TODO: Make type for clone a parameter, do not override by
		// default.
		if (action->type == ACTION_REDUCE) {
			// This should never happen in practice, but just in
			// case we have a malformed fsm we check it here.
			trace("skip", state, action, array_to_int(it.key, it.size), "reduction on first-set", 0);
			continue;
		}
		clone = c_new(Action, 1);
		clone->reduction = action->reduction;
		clone->state = action->state;
		clone->type = ACTION_CONTEXT_SHIFT;
		_state_add_buffer(state, it.key, it.size, 0, 0, clone);
		trace("add", state, action, array_to_int(it.key, it.size), "first", 0);
	}
	radix_tree_iterator_dispose(&it);
}

void state_add_reduce_follow_set(State *from, State *to, int symbol)
{
	Action *ac;
	Iterator it;

	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.
	radix_tree_iterator_init(&it, &(to->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		_state_add_buffer(from, it.key, it.size, ACTION_REDUCE, symbol, NULL);
		trace("add", from, ac, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}



//# Action functions

void action_init(Action *action, char type, int reduction, State *state)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
}


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal)
{
	radix_tree_init(&nonterminal->refs, 0, 0, NULL);
	nonterminal->status = NONTERMINAL_CLEAR;
	nonterminal->start = NULL;
	nonterminal->end = NULL;
}

void nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol)
{
	Reference *ref = c_new(Reference, 1);
	ref->state = state;
	//TODO: Is it used?
	ref->symbol = symbol;
	ref->status = REF_PENDING;
	//TODO: is ref key ok?
	radix_tree_set_intptr(&nonterminal->refs, (intptr_t)ref, ref);
	nonterminal->status |= NONTERMINAL_RETURN_REF;

	//Set end state status if exists
	if(nonterminal->end) {
		nonterminal->end->status |= STATE_RETURN_REF;
	}
}

void nonterminal_dispose(Nonterminal *nonterminal)
{
	//Delete all references
	Reference *ref;
	Iterator it;

	radix_tree_iterator_init(&it, &nonterminal->refs);
	while(ref = (Reference *)radix_tree_iterator_next(&it)) {
		c_delete(ref);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&nonterminal->refs);
}
