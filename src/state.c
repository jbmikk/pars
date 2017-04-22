#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"
#include <stdio.h>

//# State functions

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p:%i)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		T1, \
		T2, \
		T2? T2->flags: 0, \
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
	radix_tree_init(&state->actions);
	radix_tree_init(&state->refs);
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
	while((ac = (Action *)radix_tree_iterator_next(&it))) {
		c_delete(ac);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&state->actions);

	//Delete all references
	Reference *ref;
	radix_tree_iterator_init(&it, &state->refs);
	while((ref = (Reference *)radix_tree_iterator_next(&it))) {
		c_delete(ref);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&state->refs);
}

static Action *_state_get_transition(State *state, unsigned char *buffer, unsigned int size)
{
	Action *action;
	Action *range;
	
	action = (Action *)radix_tree_get(&state->actions, buffer, size);

	if(!action) {
		range = (Action *)radix_tree_get_prev(&state->actions, buffer, size);
		if(range && (range->flags & ACTION_FLAG_RANGE)) {
			int symbol = array_to_int(buffer, size);
			//TODO: Fix negative symbols in transitions.
			//negative symbols are interpreted as unsigned chars
			//when doing scans, but as signed ints when converted
			//to ints, we ignore negative numbers for now.
			if(symbol > 0 && symbol <= range->end_symbol) {
				action = range;
			}
		}
	} 
	return action;
}

static Action *_state_add_buffer(State *state, unsigned char *buffer, unsigned int size, Action *action)
{
	Action *collision = _state_get_transition(state, buffer, size);
	
	if(collision) {
		if(
			collision->type == action->type &&
			collision->reduction == action->reduction
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
	} else {
		radix_tree_set(&state->actions, buffer, size, action);
	}
	return action;
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	action_init(action, type, reduction, NULL, 0, 0);

	unsigned char buffer[sizeof(int)];
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
		if(type == ACTION_SHIFT) {
			trace("add", state, action, symbol, "shift", 0);
		} else if(type == ACTION_DROP) {
			trace("add", state, action, symbol, "drop", 0);
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

Action *state_add_range(State *state, int symbol1, int symbol2, int type, int reduction)
{
	Action *action = c_new(Action, 1);
	Action *cont;

	unsigned char buffer[sizeof(int)];

	action_init(action, type, reduction, NULL, ACTION_FLAG_RANGE, symbol2);

	int_to_padded_array(buffer, symbol1);
	cont = _state_add_buffer(state, buffer, sizeof(int), action);

	if(cont) {
		if(type == ACTION_SHIFT) {
			trace("add", state, action, symbol1, "range-shift", 0);
		} else if(type == ACTION_DROP) {
			trace("add", state, action, symbol1, "range-drop", 0);
		} else if(type == ACTION_REDUCE) {
			trace("add", state, action, symbol1, "range-reduce", 0);
		} else if(type == ACTION_ACCEPT) {
			trace("add", state, action, symbol1, "range-accept", 0);
		} else {
			trace("add", state, action, symbol1, "range-action", 0);
		}
	} else {
		c_delete(action);
	}
	return cont;
}


void reference_solve_first_set(Reference *ref, int *unsolved)
{
	Action *action, *clone;
	Iterator it;
	radix_tree_iterator_init(&it, &(ref->to_state->actions));

	while((action = (Action *)radix_tree_iterator_next(&it))) {
		//TODO: Make type for clone a parameter, do not override by
		// default.

		// When a symbol is present, assume nonterminal invocation
		int clone_type;
		if(ref->symbol) {
			if (action->type == ACTION_REDUCE) {
				// This could happen when the start state of a
				// nonterminal is also an end state.
				trace(
					"skip",
					ref->state,
					action,
					array_to_int(it.key, it.size),
					"reduction on first-set",
					0
				);
				 continue;
			}
			clone_type = ACTION_SHIFT;
		} else {
			// It could happen when merging loops in final states
			// that action->type == ACTION_REDUCE
			clone_type = action->type;
		}

		fflush(stdin);
		Action *col = _state_get_transition(ref->state, it.key, it.size);

		if(col) {
			trace(
				"collision",
				ref->state,
				col,
				array_to_int(it.key, it.size),
				"sf",
				0
			);

			//Collision detected, redefine existing action to
			//point to new merge state in order to disambiguate.
			//TODO: should only merge if actions are identical
			//TODO: Make this work with ranges.
			State *merge = c_new(State, 1);
			state_init(merge);

			//Merge first set for the actions continuations.
			state_add_reference(merge, NULL, col->state);
			state_add_reference(merge, NULL, action->state);

			//Create unified action pointing to merged state.
			action_init(col, clone_type, col->reduction, merge, col->flags, col->end_symbol);

			*unsolved = 1;
		} else {

			//No collision detected, clone the action an add it.
			clone = c_new(Action, 1);
			action_init(clone, clone_type, action->reduction, action->state, action->flags, action->end_symbol);

			trace(
				"add",
				ref->state,
				clone,
				array_to_int(it.key, it.size),
				"first-set",
				0
			);

			_state_add_buffer(ref->state, it.key, it.size, clone);
		}
		ref->status = REF_SOLVED;
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
	while((ac = (Action *)radix_tree_iterator_next(&it))) {
		Action *reduce = c_new(Action, 1);
		action_init(reduce, ACTION_REDUCE, symbol, NULL, ac->flags, ac->end_symbol);

		_state_add_buffer(from, it.key, it.size, reduce);
		trace("add", from, reduce, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}

Action *state_get_transition(State *state, int symbol)
{
	unsigned char buffer[sizeof(int)];
	int_to_padded_array(buffer, symbol);
	return _state_get_transition(state, buffer, sizeof(int));
}


//# Action functions

void action_init(Action *action, char type, int reduction, State *state, char flags, int end_symbol)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
	action->flags = flags;
	action->end_symbol = end_symbol;
}


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal)
{
	radix_tree_init(&nonterminal->refs);
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
	while((ref = (Reference *)radix_tree_iterator_next(&it))) {
		c_delete(ref);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&nonterminal->refs);
}
