#include "fsm.h"

#include <stdlib.h>

#include "fsmtrace.h"

static int _clone_action(Reference *ref, int key, Action *action)
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

		state_add_action(ref->state, key, &clone);
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

	bmap_cursor_action_init(&cursor, &(ref->to_state->actions));
	while(bmap_cursor_action_next(&cursor)) {
		entry = bmap_cursor_action_current(&cursor);
		*unsolved += _clone_action(ref, entry->key, &entry->action);
	}
	bmap_cursor_action_dispose(&cursor);

	// TODO: Maybe the reference is not always solved?
	ref->status = REF_SOLVED;
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

	state_add_reduce_follow_set(nt->end, cont->state, sb->id);
	ref->status = REF_SOLVED;
}

