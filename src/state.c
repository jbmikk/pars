#include "fsm.h"

#include "cmemory.h"
#include "rtree.h"
#include "arrays.h"
#include <stdlib.h>
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

DEFINE_BMAP_FUNCTIONS(int, Action *, Action, action, IMPLEMENTATION)

void state_init(State *state)
{
	bmap_action_init(&state->actions);
	rtree_init(&state->refs);
	state->status = STATE_CLEAR;
}

void state_add_reference(State *state, Symbol *symbol, State *to_state)
{
	Reference *ref = malloc(sizeof(Reference));
	ref->state = state;
	ref->to_state = to_state;
	//TODO: Is it really necessary? Not used right now.
	ref->symbol = symbol;
	ref->status = REF_PENDING;

	//Is ref key ok?
	rtree_set_intptr(&state->refs, (intptr_t)ref, ref);
	state->status |= STATE_INVOKE_REF;
}

void state_dispose(State *state)
{
	BMapCursorAction cursor;

	//Delete all actions
	Action *ac;
	bmap_cursor_action_init(&cursor, &state->actions);
	while(bmap_cursor_action_next(&cursor)) {
		ac = bmap_cursor_action_current(&cursor)->action;
		free(ac);
	}
	bmap_cursor_action_dispose(&cursor);

	bmap_action_dispose(&state->actions);

	//Delete all references
	Reference *ref;
	Iterator it;
	rtree_iterator_init(&it, &state->refs);
	while((ref = (Reference *)rtree_iterator_next(&it))) {
		free(ref);
	}
	rtree_iterator_dispose(&it);

	rtree_dispose(&state->refs);
}

static Action *_state_get_transition(State *state, int symbol)
{
	Action *action = NULL;
	Action *range;
	
	BMapEntryAction *entry = bmap_action_get(&state->actions, symbol);

	if(!entry) {
		entry = bmap_action_get_lt(&state->actions, symbol);
		range = entry? entry->action: NULL;
		if(range && (range->flags & ACTION_FLAG_RANGE)) {
			//TODO: Fix negative symbols in transitions.
			//negative symbols are interpreted as unsigned chars
			//when doing scans, but as signed ints when converted
			//to ints, we ignore negative numbers for now.
			if(symbol > 0 && symbol <= range->end_symbol) {
				action = range;
			}
		}
	} else {
		action = entry->action;
	}
	return action;
}

static Action *_state_add_buffer(State *state, int symbol, Action *action)
{
	Action *collision = _state_get_transition(state, symbol);
	
	if(collision) {
		if(
			collision->type == action->type &&
			collision->reduction == action->reduction
		) {
			trace(
				"dup",
				state,
				action,
				symbol,
				"skip",
				action->reduction
			);
		} else {
			trace(
				"dup",
				state,
				action,
				symbol,
				"conflict",
				action->reduction
			);
			//TODO: add sentinel ?
		}
		free(action);
		action = NULL;
	} else {
		bmap_action_insert(&state->actions, symbol, action);
	}
	return action;
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action * action = malloc(sizeof(Action));
	action_init(action, type, reduction, NULL, 0, 0);

	action = _state_add_buffer(state, symbol, action);

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

Action *state_add_range(State *state, Range range, int type, int reduction)
{
	Action *action = malloc(sizeof(Action));
	Action *cont;

	action_init(action, type, reduction, NULL, ACTION_FLAG_RANGE, range.end);

	cont = _state_add_buffer(state, range.start, action);

	if(cont) {
		if(type == ACTION_SHIFT) {
			trace("add", state, action, range.start, "range-shift", 0);
		} else if(type == ACTION_DROP) {
			trace("add", state, action, range.start, "range-drop", 0);
		} else if(type == ACTION_REDUCE) {
			trace("add", state, action, range.start, "range-reduce", 0);
		} else if(type == ACTION_ACCEPT) {
			trace("add", state, action, range.start, "range-accept", 0);
		} else {
			trace("add", state, action, range.start, "range-action", 0);
		}
	} else {
		free(action);
	}
	return cont;
}


void reference_solve_first_set(Reference *ref, int *unsolved)
{
	Action *action, *clone;
	BMapCursorAction cursor;
	BMapEntryAction *entry;
	bmap_cursor_action_init(&cursor, &(ref->to_state->actions));

	while(bmap_cursor_action_next(&cursor)) {
		entry = bmap_cursor_action_current(&cursor);
		action = entry->action;
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
					entry->key,
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
		Action *col = _state_get_transition(ref->state, entry->key);

		if(col) {
			if(
				col->type == action->type &&
				col->state == action->state &&
				col->reduction == action->reduction &&
				col->end_symbol == action->end_symbol &&
				col->flags == action->flags
			) {
				trace(
					"collision",
					ref->state,
					action,
					entry->key,
					"skip duplicate",
					0
				);
				continue;
			}

			if(col->state == NULL || action->state == NULL) {
				trace(
					"collision",
					ref->state,
					action,
					entry->key,
					"unhandled",
					0
				);
				continue;
			}

			//Collision: redefine actions in order to disambiguate.
			trace(
				"collision",
				ref->state,
				col,
				entry->key,
				"deambiguate state",
				0
			);

			//TODO: should only merge if actions are identical
			//TODO: Make this work with ranges.
			State *merge = malloc(sizeof(State));
			state_init(merge);

			//Merge first set for the actions continuations.
			state_add_reference(merge, NULL, col->state);
			state_add_reference(merge, NULL, action->state);

			//Create unified action pointing to merged state.
			action_init(col, clone_type, col->reduction, merge, col->flags, col->end_symbol);

			*unsolved = 1;
		} else {

			//No collision detected, clone the action an add it.
			clone = malloc(sizeof(Action));
			action_init(clone, clone_type, action->reduction, action->state, action->flags, action->end_symbol);

			trace(
				"add",
				ref->state,
				clone,
				entry->key,
				"first-set",
				0
			);

			_state_add_buffer(ref->state, entry->key, clone);
		}
		ref->status = REF_SOLVED;
	}
	bmap_cursor_action_dispose(&cursor);
}

void state_add_reduce_follow_set(State *from, State *to, int symbol)
{
	Action *ac;
	BMapCursorAction cursor;
	BMapEntryAction *entry;

	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.
	bmap_cursor_action_init(&cursor, &to->actions);
	while(bmap_cursor_action_next(&cursor)) {
		entry = bmap_cursor_action_current(&cursor);
		ac = entry->action;
		Action *reduce = malloc(sizeof(Action));
		action_init(reduce, ACTION_REDUCE, symbol, NULL, ac->flags, ac->end_symbol);

		_state_add_buffer(from, entry->key, reduce);
		trace("add", from, reduce, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	bmap_cursor_action_dispose(&cursor);
}

Action *state_get_transition(State *state, int symbol)
{
	return _state_get_transition(state, symbol);
}


//# Action functions

void action_init(Action *action, char type, int reduction, State *state, char flags, int end_symbol)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
	action->flags = flags;
	action->end_symbol = end_symbol;
	action->mode = 0;
}


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal)
{
	rtree_init(&nonterminal->refs);
	nonterminal->status = NONTERMINAL_CLEAR;
	nonterminal->start = NULL;
	nonterminal->end = NULL;
	nonterminal->mode = 0;
	nonterminal->pushes_mode = 0;
	nonterminal->pops_mode = 0;
}

void nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol)
{
	Reference *ref = malloc(sizeof(Reference));
	ref->state = state;
	ref->to_state = NULL;
	//TODO: Is it used?
	ref->symbol = symbol;
	ref->status = REF_PENDING;
	//TODO: is ref key ok?
	rtree_set_intptr(&nonterminal->refs, (intptr_t)ref, ref);
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

	rtree_iterator_init(&it, &nonterminal->refs);
	while((ref = (Reference *)rtree_iterator_next(&it))) {
		free(ref);
	}
	rtree_iterator_dispose(&it);

	rtree_dispose(&nonterminal->refs);
}
