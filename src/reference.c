#include "fsm.h"

#include <stdlib.h>

#include "fsmtrace.h"

static void _merge_action_set(State *to, BMapAction *action_set)
{
	BMapCursorAction cursor;
	BMapEntryAction *entry;

	// Merge set
	bmap_cursor_action_init(&cursor, action_set);
	while(bmap_cursor_action_next(&cursor)) {
		entry = bmap_cursor_action_current(&cursor);
		state_append_action(to, entry->key, &entry->action);
	}
	bmap_cursor_action_dispose(&cursor);
}

static int _clone_fs_action(BMapAction *action_set, Reference *ref, int key, Action *action)
{
	int unsolved = 0;
	//TODO: Make type for clone a parameter, do not override by
	// default.

	// When a symbol is present, assume nonterminal invocation
	int clone_type;
	if(ref->symbol) {
		if (action->type == ACTION_REDUCE) {
			// This could happen when the start state of a
			// nonterminal is also an end state.
			trace_op(
				"skip",
				ref->state,
				action,
				key,
				"reduction on first-set",
				0
			);
			goto end;
		}
		clone_type = ACTION_SHIFT;
	} else {
		// It could happen when merging loops in final states
		// that action->type == ACTION_REDUCE
		clone_type = action->type;
	}

	Action *col = state_get_transition(ref->state, key);

	// TODO: Unify collision detection, skipping and merging with the ones
	// used in the state functions.
	if(col) {
		if(
			col->type == action->type &&
			col->state == action->state &&
			col->reduction == action->reduction &&
			col->end_symbol == action->end_symbol &&
			col->flags == action->flags
		) {
			trace_op(
				"collision",
				ref->state,
				action,
				key,
				"skip duplicate",
				0
			);
			goto end;
		}

		if(col->state == NULL || action->state == NULL) {
			trace_op(
				"collision",
				ref->state,
				action,
				key,
				"unhandled",
				0
			);
			goto end;
		}

		//Collision: redefine actions in order to disambiguate.
		trace_op(
			"collision",
			ref->state,
			col,
			key,
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

		//TODO: Is it necessary to return the unsolved count?
		// Shouldn't it be enough to have further references pushed?
		unsolved = 1;
	} else {

		//No collision detected, clone the action an add it.
		Action clone;
		action_init(&clone, clone_type, action->reduction, action->state, action->flags, action->end_symbol);

		trace_op(
			"add",
			ref->state,
			&clone,
			key,
			"first-set",
			0
		);

		bmap_action_m_append(action_set, key, clone);
	}
end:
	return unsolved;
}

void reference_solve_first_set(Reference *ref, int *unsolved)
{
	BMapCursorAction cursor;
	BMapEntryAction *entry;

	if(ref->status == REF_SOLVED) {
		//ref already solved
		return;
	}

	if(ref->to_state->status != STATE_CLEAR) {
		trace_state(
			"skip first set from",
			ref->to_state,
			""
		);
		*unsolved = 1;
		return;
	}

	//solve reference
	trace_state(
		"append first set from",
		ref->to_state,
		""
	);

	// Add all cloned actions into a clone set, then merge the clone set
	// into the ref state. It's important to do this in two passes in 
	// order to avoid having collisions produced by the very actions we
	// are cloning.
	BMapAction action_set;
	bmap_action_init(&action_set);

	bmap_cursor_action_init(&cursor, &(ref->to_state->actions));
	while(bmap_cursor_action_next(&cursor)) {
		entry = bmap_cursor_action_current(&cursor);
		*unsolved += _clone_fs_action(&action_set, ref, entry->key, &entry->action);
	}
	bmap_cursor_action_dispose(&cursor);

	// TODO: should it always merge?
	_merge_action_set(ref->state, &action_set);
	bmap_action_dispose(&action_set);

	// TODO: Maybe the reference is not always solved?
	ref->status = REF_SOLVED;
}

static void _clone_rs_action(Reference *ref, BMapAction *action_set, Nonterminal *nt, int key, Action *action)
{
	Symbol *sb = ref->symbol;
	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.

	// Always reduce?
	int clone_type = ACTION_REDUCE;

	Action *col = state_get_transition(nt->end, key);

	// TODO: Unify collision detection, skipping and merging with the ones
	// used in the state functions.
	if(col) {
		if(
			col->type == action->type &&
			col->state == action->state &&
			col->reduction == action->reduction &&
			col->end_symbol == action->end_symbol &&
			col->flags == action->flags
		) {
			trace_op(
				"collision",
				nt->end,
				action,
				key,
				"skip duplicate",
				0
			);
			goto end;
		}

		//Collision: redefine actions in order to disambiguate.
		trace_op(
			"collision",
			nt->end,
			col,
			key,
			"unhandled return set collision",
			0
		);

		// TODO: sentinel?
	} else {

		//No collision detected, clone the action an add it.
		Action clone;
		action_init(&clone, clone_type, sb->id, NULL, action->flags, action->end_symbol);

		trace_op(
			"add",
			nt->end,
			&clone,
			key,
			"return-set",
			0
		);

		bmap_action_m_append(action_set, key, clone);
	}
end:
	return;
}

void reference_solve_return_set(Reference *ref, Nonterminal *nt, int *unsolved)
{
	Symbol *sb = ref->symbol;

	if(ref->status == REF_SOLVED) {
		//Ref already solved
		return;
	}

	BMapEntryAction *entry = bmap_action_get(&ref->state->actions, sb->id);
	Action *cont = entry? &entry->action: NULL;

	//There could be many references here:
	// * When the calling NT's end state matches the continuation, there
	//   could be many references to that terminal, we need the whole 
	//   follow set.
	// * When the continuation has its own references to other NT's.
	//   In this case those invokes have to be solved to get the followset.

	// TODO: All references must be solved! missing continuation return refs
	if(cont && cont->state->status != STATE_CLEAR) {
		trace_state(
			"skip return ref to",
			cont->state,
			""
		);
		*unsolved = 1;
		return;
	}

	//Solve reference
	trace_state(
		"append return ref to",
		cont->state,
		""
	);

	BMapCursorAction cursor;

	BMapAction action_set;
	bmap_action_init(&action_set);

	bmap_cursor_action_init(&cursor, &cont->state->actions);
	while(bmap_cursor_action_next(&cursor)) {
		BMapEntryAction *e = bmap_cursor_action_current(&cursor);
		_clone_rs_action(ref, &action_set, nt, e->key, &e->action);
	}
	bmap_cursor_action_dispose(&cursor);

	_merge_action_set(nt->end, &action_set);
	bmap_action_dispose(&action_set);

	ref->status = REF_SOLVED;
}
