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
	int result = REF_RESULT_SOLVED;
	//TODO: Make type for clone a parameter, do not override by
	// default.

	// When a symbol is present, assume nonterminal invocation
	int clone_type;

	if(ref->type == REF_TYPE_DEFAULT) {
		// It could happen when merging loops in final states
		// that action->type == ACTION_REDUCE
		clone_type = action->type;
	} else if(ref->type == REF_TYPE_SHIFT) {
		// TODO: Maybe shifting should decorate other actions?
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
		} else if (
			action->type == ACTION_POP ||
			action->type == ACTION_POP_SHIFT
		) {
			clone_type = ACTION_POP_SHIFT;
		} else {
			clone_type = ACTION_SHIFT;
		}
	} else if(ref->type == REF_TYPE_START) {
		if (action->type == ACTION_REDUCE || action->type == ACTION_ACCEPT) {
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
		clone_type = ACTION_START;
	}

	//TODO: Detect collision for ranges (only testing `key` for now)
	// We have to iterate all the range collisions and create a merged
	// transition for each one.
	Action *col = state_get_transition(ref->state, key);

	// TODO: Unify collision detection, skipping and merging with the ones
	// used in the state functions.
	if(col && ref->strategy == REF_STRATEGY_MERGE) {
		printf("COL\n");
		if(!action_compare(*action, *col)) {
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

		printf("MERGE %i %c\n", key, key);
		//Merge first set for the actions continuations.
		state_add_reference(merge, REF_TYPE_DEFAULT, REF_STRATEGY_MERGE, NULL, col->state);
		state_add_reference(merge, REF_TYPE_DEFAULT, REF_STRATEGY_MERGE, NULL, action->state);

		//Create unified action pointing to merged state.
		action_init(col, clone_type, col->reduction, merge, col->flags, col->end_symbol);

		result = REF_RESULT_CHANGED;
	} else {

		printf("NO COL\n");
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

		bmap_action_m_prepend(action_set, key, clone);
	}
end:
	return result;
}

// TODO: Make it use from and to states like _merge_deep_next
State *_deep_clone_next(State *state, BMapState *cloned, State *end, State *sibling_end, State *cont)
{
	BMapEntryState *in_states = bmap_state_get(cloned, (intptr_t)state);
	State *clone;

	if(!in_states) {
		clone = malloc(sizeof(State));
		state_init(clone);
		// TODO: are there other places that need this?
		clone->flags = state->flags;

		if(state == end) {
			// TODO: Fix possible cont leak?
			state_add_reference(clone, REF_TYPE_DEFAULT, REF_STRATEGY_SPLIT, NULL, cont);
		}

		// When the start/cont state is also the end state for the 
		// nonterminal the previous state for the loop also becomes a 
		// possible end state. When the copy operator clones the end 
		// state it adds references to the copy continuation, but only 
		// for the default end state, ignoring this particular case. 
		// In order to fix this, we need to add a ref from the sibling
		// end to the continuation
		if(state == sibling_end) {

			// TODO: Fix possible cont leak?
			state_add_reference(clone, REF_TYPE_DEFAULT, REF_STRATEGY_SPLIT, NULL, cont);
		}

		//TODO: Check insert errors
		bmap_state_insert(cloned, (intptr_t)state, clone);

		BMapCursorAction cursor;
		bmap_cursor_action_init(&cursor, &state->actions);
		while(bmap_cursor_action_next(&cursor)) {
			BMapEntryAction *entry;
			entry = bmap_cursor_action_current(&cursor);
			Action ac = entry->action;
			if(ac.state) {
				ac.state = _deep_clone_next(ac.state, cloned, end, sibling_end, cont);
			}
			// Skip accept to avoid problems with lexer_nonterminal
			if(ac.type != ACTION_ACCEPT) {
				bmap_action_m_prepend(&clone->actions, entry->key, ac);
			}
		}
		bmap_cursor_action_dispose(&cursor);
	} else {
		clone = in_states->state;
	}
	return clone;
}

static int _clone_deep(Reference *ref)
{
	int result = REF_RESULT_SOLVED;

	// TODO detect collisions between states' action sets
	//Action *col = state_get_transition(ref->state, key);

	// TODO: Unify collision detection, skipping and merging with the ones
	// used in the state functions.
	//if(col) {
		// TODO: manage collisions
	//} else {
		// TODO: if all states ready proceed, otherwise defer
		BMapState walked_states;
		bmap_state_init(&walked_states);
		bool all_ready = state_all_ready(ref->to_state, &walked_states);
		bmap_state_dispose(&walked_states);
		if(!all_ready) {
			result = REF_RESULT_PENDING;
			goto end;
		}

		//No collision detected, clone the action an add it.
		BMapState cloned_states;
		bmap_state_init(&cloned_states);

		State *cloned = _deep_clone_next(ref->to_state, &cloned_states, ref->nonterminal->end, ref->nonterminal->sibling_end, ref->cont);

		bmap_state_dispose(&cloned_states);

		// Merge cloned fragment
		_merge_action_set(ref->state, &cloned->actions);

		// TODO: Improve cloning, unnecessary malloc/free
		// Delete root state, we only need its action set
		state_dispose(cloned);
		free(cloned);

		// Because deep cloning always creates a cont ref (for now)
		result = REF_RESULT_CHANGED;
		ref->status = REF_SOLVED;
	//}
end:
	return result;
}

int _merge_deep_next(State *from, State *to, BMapState *cloned)
{
	BMapCursorAction cursor;
	bmap_cursor_action_init(&cursor, &to->actions);
	while(bmap_cursor_action_next(&cursor)) {
		BMapEntryAction *entry;
		entry = bmap_cursor_action_current(&cursor);
		Action ac = entry->action;
		if(ac.state) {
			BMapEntryState *in_states = bmap_state_get(cloned, (intptr_t)ac.state);
			State *clone;
			if(in_states) {
				clone = in_states->state;
			} else {
				clone = malloc(sizeof(State));
				state_init(clone);
				// TODO: are there other places that need this?
				clone->flags = ac.state->flags;

				//TODO: Check and return errors
				bmap_state_insert(cloned, (intptr_t)ac.state, clone);
				_merge_deep_next(clone, ac.state, cloned);
			}
			ac.state = clone;
		}
		bmap_action_m_prepend(&from->actions, entry->key, ac);
	}
	bmap_cursor_action_dispose(&cursor);
	return 0;
}

static int _ref_merge_deep(Reference *ref)
{
	int result = REF_RESULT_SOLVED;

	BMapState walked_states;
	bmap_state_init(&walked_states);
	bool all_ready = state_all_ready(ref->to_state, &walked_states);
	bmap_state_dispose(&walked_states);
	if(!all_ready) {
		result = REF_RESULT_PENDING;
		goto end;
	}

	//No collision detected, clone the action an add it.
	BMapState cloned_states;
	bmap_state_init(&cloned_states);

	_merge_deep_next(ref->state, ref->to_state, &cloned_states);

	bmap_state_dispose(&cloned_states);

	// Because deep cloning always creates a cont ref (for now)
	result = REF_RESULT_CHANGED;
	ref->status = REF_SOLVED;
end:
	return result;
}

void reference_solve_first_set(Reference *ref, int *result)
{
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
		*result = REF_RESULT_PENDING;
		return;
	}

	//solve reference
	if(ref->type == REF_TYPE_COPY) {
		trace_state("first-set deep copy from", ref->to_state, "");
		*result |= _clone_deep(ref);
	} else if(ref->type == REF_TYPE_COPY_MERGE) {
		trace_state("first-set merge from", ref->to_state, "");
		*result |= _ref_merge_deep(ref);
	} else {
		trace_state("first-set clone from", ref->to_state, "");
		// Add all cloned actions into a clone set, then merge the clone set
		// into the ref state. It's important to do this in two passes in 
		// order to avoid having collisions produced by the very actions we
		// are cloning.
		BMapAction action_set;
		bmap_action_init(&action_set);

		BMapCursorAction cursor;
		BMapEntryAction *entry;

		bmap_cursor_action_init(&cursor, &(ref->to_state->actions));
		while(bmap_cursor_action_next(&cursor)) {
			entry = bmap_cursor_action_current(&cursor);
			*result |= _clone_fs_action(&action_set, ref, entry->key, &entry->action);
		}
		bmap_cursor_action_dispose(&cursor);

		if(!(*result & REF_RESULT_PENDING)) {
			_merge_action_set(ref->state, &action_set);
			ref->status = REF_SOLVED;
		}

		bmap_action_dispose(&action_set);
	}
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
		if(!action_compare(*action, *col)) {
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

		// TODO: Empty transitions should not be cloned.
		// They should be followed recursively to get the whole
		// follow set.
		// This happens when:
		// * A nonterminal is invoked at the end of a repetition and
		//   the repetition is within a group. An empty transition
		//   will be created between the repetition and the group,
		//   when the nonterminal is solved the empty transition will
		//   be found in the follow-set.

		trace_op(
			"add",
			nt->end,
			&clone,
			key,
			"return-set",
			0
		);

		bmap_action_m_prepend(action_set, key, clone);
	}
end:
	return;
}

void reference_solve_return_set(Reference *ref, Nonterminal *nt, int *result)
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
	//   In this case those refs have to be solved to get the followset.

	// TODO: All references must be solved! missing continuation return refs
	if(cont && cont->state->status != STATE_CLEAR) {
		trace_state(
			"skip return ref to",
			cont->state,
			""
		);
		*result = REF_RESULT_PENDING;
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
