#include "fsm.h"

#include "cmemory.h"
#include "rtree.h"
#include "arrays.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "fsmtrace.h"

//# State functions

FUNCTIONS(BMap, int, Action, Action, action)

FUNCTIONS(BMap, intptr_t, Reference*, Reference, ref)

void state_init(State *state)
{
	bmap_action_init(&state->actions);
	bmap_ref_init(&state->refs);
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
	//TODO: Can refs be overwritten? Possible leak!
	bmap_ref_insert(&state->refs, (intptr_t)ref, ref);
	state->status |= STATE_INVOKE_REF;
}

void state_dispose(State *state)
{
	bmap_action_dispose(&state->actions);

	//Delete all references
	Reference *ref;
	BMapCursorReference rcursor;
	bmap_cursor_ref_init(&rcursor, &state->refs);
	while(bmap_cursor_ref_next(&rcursor)) {
		ref = bmap_cursor_ref_current(&rcursor)->ref;
		free(ref);
	}
	bmap_cursor_ref_dispose(&rcursor);

	bmap_ref_dispose(&state->refs);
}

static Action *_state_get_transition(State *state, int symbol)
{
	Action *action = NULL;
	Action *range;
	
	BMapEntryAction *entry = bmap_action_m_get(&state->actions, symbol);
	BMapCursorAction cur;

	if(!entry) {
		// TODO: We should have a loop to recursively test outer 
		// ranges. The first match wins.
		bmap_cursor_action_init(&cur, &state->actions);
		bmap_cursor_action_revert(&cur);
		bmap_cursor_action_move_gt(&cur, symbol);

		while(bmap_cursor_action_next(&cur)) {
			entry = bmap_cursor_action_current(&cur);
			range = &entry->action;
			if(range && (range->flags & ACTION_FLAG_RANGE)) {
				//TODO: Fix negative symbols in transitions.
				//negative symbols are interpreted as unsigned chars
				//when doing scans, but as signed ints when converted
				//to ints, we ignore negative numbers for now.
				if(symbol > 0 && symbol <= range->end_symbol) {
					action = range;
					break;
				}
			}
		}
		bmap_cursor_action_dispose(&cur);

	} else {
		action = &entry->action;
	}
	return action;
}

static Action *_state_add_action(State *state, int symbol, Action *action)
{
	Action *collision = _state_get_transition(state, symbol);
	Action *ret;
	
	// TODO: Compare all action properties?
	// TODO: Unify collision detection, skipping and merging strategies
	// with the ones used in the reference functions.
	bool equal = 
		collision &&
		collision->type == action->type &&
		collision->reduction == action->reduction &&
		collision->end_symbol == action->end_symbol;

	if(equal) {
		trace_op(
			"dup",
			state,
			action,
			symbol,
			"skip",
			action->reduction
		);
		ret = NULL;
	} else {
		// TODO: Detect conflicts later?
		if (collision) {
			trace_op(
				"dup",
				state,
				action,
				symbol,
				"conflict",
				action->reduction
			);
		}
		BMapEntryAction *entry;
		entry = bmap_action_m_append(&state->actions, symbol, *action);
		ret = &entry->action;
	}
	return ret;
}

Action *state_append_action(State *state, int symbol, Action *action)
{
	BMapEntryAction *entry;
	entry = bmap_action_m_append(&state->actions, symbol, *action);
	return &entry->action;
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action ac;
	action_init(&ac, type, reduction, NULL, 0, 0);

	Action *action = _state_add_action(state, symbol, &ac);

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
			trace_op("add", state, action, symbol, "shift", 0);
		} else if(type == ACTION_DROP) {
			trace_op("add", state, action, symbol, "drop", 0);
		} else if(type == ACTION_REDUCE) {
			trace_op("add", state, action, symbol, "reduce", 0);
		} else if(type == ACTION_ACCEPT) {
			trace_op("add", state, action, symbol, "accept", 0);
		} else {
			trace_op("add", state, action, symbol, "action", 0);
		}
	}
	return action;
}

Action *state_add_range(State *state, Range range, int type, int reduction)
{
	Action ac;
	Action *action;

	action_init(&ac, type, reduction, NULL, ACTION_FLAG_RANGE, range.end);

	action = _state_add_action(state, range.start, &ac);

	if(action) {
		if(type == ACTION_SHIFT) {
			trace_op("add", state, action, range.start, "range-shift", 0);
		} else if(type == ACTION_DROP) {
			trace_op("add", state, action, range.start, "range-drop", 0);
		} else if(type == ACTION_REDUCE) {
			trace_op("add", state, action, range.start, "range-reduce", 0);
		} else if(type == ACTION_ACCEPT) {
			trace_op("add", state, action, range.start, "range-accept", 0);
		} else {
			trace_op("add", state, action, range.start, "range-action", 0);
		}
	}
	return action;
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
		ac = &entry->action;
		Action reduce;
		action_init(&reduce, ACTION_REDUCE, symbol, NULL, ac->flags, ac->end_symbol);

		_state_add_action(from, entry->key, &reduce);
		trace_op("add", from, &reduce, entry->key, "reduce-follow", symbol);
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
	bmap_ref_init(&nonterminal->refs);
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
	bmap_ref_insert(&nonterminal->refs, (intptr_t)ref, ref);
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
	BMapCursorReference rcursor;

	bmap_cursor_ref_init(&rcursor, &nonterminal->refs);
	while(bmap_cursor_ref_next(&rcursor)) {
		ref = bmap_cursor_ref_current(&rcursor)->ref;
		free(ref);
	}
	bmap_cursor_ref_dispose(&rcursor);

	bmap_ref_dispose(&nonterminal->refs);
}
