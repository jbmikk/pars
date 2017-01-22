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

static Action *_state_add_buffer(State *state, unsigned char *buffer, unsigned int size, Action *action)
{
	Action *prev = (Action *)radix_tree_try_set(&state->actions, buffer, size, action);
	if(prev) {
		if(
			prev->type == action->type &&
			prev->reduction == action->reduction
		) {
			trace(
				"dup",
				state,
				action,
				array_to_int(buffer, size),
				"skip",
				action->reduction
			);
		} else {
			trace(
				"dup",
				state,
				action,
				array_to_int(buffer, size),
				"conflict",
				action->reduction
			);
			//TODO: add sentinel ?
		}
		c_delete(action);
		action = NULL;
	}
	return action;
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	action_init(action, type, reduction, NULL);

	unsigned char buffer[sizeof(int)];
	unsigned int size;
	int_to_padded_array(buffer, symbol);

	action = _state_add_buffer(state, buffer, sizeof(int), action);

	//TODO: Ambiguos transitions should be handled properly.
	// There are different types of conflicts that could arise with 
	// different implications.
	// SHIFT/SHIFT: they could happen within a nonterminal or when merging
	// follow sets for other nonterminals. New states should be created
	// until ambiguity is solved. If the nonterminals are ambiguos until
	// their end state, then the final state is also duplicated, including
	// only the reductions belonging to the place of invocation.
	// REDUCE/REDUCE: for now we don't unify nonterminals with identical
	// states ahead of time, we only do it when they are invoked at the
	// same location. If two nonterminals are ambiguos until reaching the
	// final state and both are invoked at the same location we are going
	// to generate a whole set of ambiguos states with the same followset,
	// in this case we are going to have a REDUCE/REDUCE conflict that 
	// cannot be avoided.
	if(action) {
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
	}
	return action;
}

void state_add_first_set(State *state, State* source, Symbol *symbol)
{
	Action *action, *clone;
	Iterator it;
	radix_tree_iterator_init(&it, &(source->actions));
	while(action = (Action *)radix_tree_iterator_next(&it)) {
		//TODO: Make type for clone a parameter, do not override by
		// default.
		clone = c_new(Action, 1);
		clone->reduction = action->reduction;
		clone->state = action->state;
		// When a symbol is present, assume nonterminal invocation
		if(symbol) {
			if (action->type == ACTION_REDUCE) {
				// This could happen when the start state of a
				// nonterminal is also an end state.
				trace(
					"skip",
					state,
					action,
					array_to_int(it.key, it.size),
					"reduction on first-set",
					0
				);
				 continue;
			}
			clone->type = ACTION_CONTEXT_SHIFT;
		} else {
			// It could happen when merging loops in final states
			// that action->type == ACTION_REDUCE
			clone->type = action->type;
		}
		_state_add_buffer(state, it.key, it.size, clone);
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
		Action *reduce = c_new(Action, 1);
		action_init(reduce, ACTION_REDUCE, symbol, NULL);

		_state_add_buffer(from, it.key, it.size, reduce);
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
	ref->to_state = NULL;
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
