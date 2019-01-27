#include "fsm.h"

#include <stdlib.h>
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

void state_add_reference(State *state, char type, Symbol *symbol, State *to_state)
{
	Reference *ref = malloc(sizeof(Reference));
	ref->state = state;
	ref->to_state = to_state;
	//TODO: Is it really necessary? Not used right now.
	ref->symbol = symbol;
	ref->status = REF_PENDING;
	ref->type = type;

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

static Action *_state_get_transition(State *state, int symbol, int path)
{
	Action *action = NULL;
	Action *range;
	
	BMapEntryAction *entry = bmap_action_m_get(&state->actions, symbol);
	BMapCursorAction cur;

	int count = 0;

	if(!entry || path != 0) {
		if(entry) {
			count++;
		}
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
					if(count < path) {
						count++;
					} else {
						action = range;
						break;
					}
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
	// TODO: What if there are more paths?
	Action *collision = _state_get_transition(state, symbol, 0);
	Action *ret;
	
	// TODO: Compare all action properties?
	// TODO: Unify collision detection, skipping and merging strategies
	// with the ones used in the reference functions. Possible remove
	// collision detection altogether at this level. We should just let
	// everything be merged because we support multi-keys. Ambiguity
	// should be handled at a higher level, at the reference level for
	// instance. And if the strategy allows having alternate paths, then
	// at the validation or execution/interpreter level we should handle
	// the multiple paths, such as when backtracking.
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

Action *state_get_transition(State *state, int symbol)
{
	return _state_get_transition(state, symbol, 0);
}

Action *state_get_path_transition(State *state, int symbol, int path)
{
	return _state_get_transition(state, symbol, path);
}

int state_solve_references(State *state) {
	int unsolved = 0;
	Reference *ref;

	if(state->status == STATE_CLEAR) {
		goto end;
	}

	trace_state("solve refs for state", state, "");

	BMapCursorReference rcursor;
	bmap_cursor_ref_init(&rcursor, &state->refs);
	while(bmap_cursor_ref_next(&rcursor)) {
		ref = bmap_cursor_ref_current(&rcursor)->ref;
		reference_solve_first_set(ref, &unsolved);
	}
	bmap_cursor_ref_dispose(&rcursor);

	if(!unsolved) {
		state->status &= ~STATE_INVOKE_REF;
		trace_state(
			"state refs clear",
			state,
			""
		);
	}

end:
	return unsolved;
}
