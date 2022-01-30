#include "fsmbuilder.h"

#include "cmemory.h"
#include "arrays.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fsmtrace.h"
#include "dbg.h"


void fsm_builder_init(FsmBuilder *builder, Fsm *fsm, char ref_strategy)
{
	builder->fsm = fsm;
	builder->action = NULL;
	builder->state = NULL;
	builder->sibling = NULL;
	builder->stack = NULL;
	builder->last_symbol = NULL;
	builder->last_nonterminal = NULL;
	builder->current_mode = 0;
	builder->ref_strategy = ref_strategy;
}

void fsm_builder_dispose(FsmBuilder *builder)
{
	//TODO: In case of interrupted fsm construction some states in the
	// stack should be deleted too.
	//TODO: write test for stack disposal
	while(builder->stack) {
		FsmFrame *frame = builder->stack;
		builder->stack = frame->next;
		free(frame);
	}
	builder->stack = NULL;
	builder->action = NULL;
	builder->state = NULL;
	builder->fsm = NULL;
}

static void _push_frame(FsmBuilder *builder, State *start, State *cont, State *sibling) 
{
	FsmFrame *frame = malloc(sizeof(FsmFrame));
	frame->start = start;
	frame->continuation = cont;
	frame->sibling = sibling;
	frame->next = builder->stack;
	builder->stack = frame;
}

static void _pop_frame(FsmBuilder *builder) 
{

	FsmFrame *frame = builder->stack;
	builder->stack = frame->next;
	free(frame);
}

static void _append_state(FsmBuilder *builder, State *state)
{
	if(builder->action) {
		builder->action->state = state;
	}
	builder->state = state;
}

static void _move_to(FsmBuilder *builder, State *state)
{
	builder->action = NULL;
	builder->state = state;
}

static void _set_sibling(FsmBuilder *builder, State *state)
{
	builder->sibling = state;
}

static void _transition(FsmBuilder *builder, Action *action)
{
	builder->action = action;
	builder->state = NULL;
	builder->sibling = NULL;
}

static void _ensure_state(FsmBuilder *builder)
{
	if(!builder->state) {
		State *state = malloc(sizeof(State));
		state_init(state);
		_append_state(builder, state);
	}
}

static Action *_add_empty(FsmBuilder *builder, int type, int reduction)
{
	Fsm *fsm = builder->fsm;
	Symbol *symbol = symbol_table_add(fsm->table, "__empty", 7);
	
	_ensure_state(builder);
	Action *action = state_add(builder->state, symbol->id, type, reduction);
	_transition(builder, action);

	return action;
}

static Action *_add_partial(FsmBuilder *builder, int type, int reduction)
{
	Fsm *fsm = builder->fsm;
	Symbol *symbol = symbol_table_add(fsm->table, "__partial", 9);

	_ensure_state(builder);
	builder->state->flags = STATE_FLAG_PARTIAL;
	Action *action = state_add(builder->state, symbol->id, type, reduction);
	_transition(builder, action);

	return action;
}

static void _reset(FsmBuilder *builder) 
{
	FsmFrame *frame = builder->stack;
	_move_to(builder, frame->start);
	_set_sibling(builder, frame->sibling);
}

static void _join_continuation(FsmBuilder *builder)
{
	FsmFrame *frame = builder->stack;

	if (builder->state) {
		// The action is already pointing to a state.
		// We can't point it to another one without losing the current
		// state.
		// The solution is to have the continuation's first set merged
		// into the current state. The continuation is likely not
		// built yet, so we have to either delay the merging until the
		// continuation is ready or add merge them now through an empty
		// transition.
		// For now we add the empty transition.
		// This happens in the following cases:
		// * When the end of a loop group meets the end of the group.
		//   First the loop ends and is joined with its continuation.
		//   Then when the outer group tries to join we already have
		//   a state in place, and we must merge the states.
		_add_empty(builder, ACTION_EMPTY, NONE);
	}
	_append_state(builder, frame->continuation);

	//TODO: Add trace for other types of operations or move to actions?
	//trace("add", NULL, builder->current, 0, "join", 0);
}

void fsm_builder_group_start(FsmBuilder *builder)
{
	_ensure_state(builder);

	State *start = builder->state;
	State *cont = malloc(sizeof(State));
	state_init(cont);

	trace_state("add", cont, "continuation");
	_push_frame(builder, start, cont, NULL);
}

void fsm_builder_group_end(FsmBuilder *builder)
{
	_join_continuation(builder);
	_pop_frame(builder);
}

void fsm_builder_loop_group_start(FsmBuilder *builder)
{
	_ensure_state(builder);

	State *start = malloc(sizeof(State));
	state_init(start);

	State *cont = start;
	State *preloop = builder->state;

	state_add_reference(preloop, REF_TYPE_DEFAULT, builder->ref_strategy, NULL, start);

	trace_state("push", cont, "continuation");
	_push_frame(builder, start, cont, preloop);

	_move_to(builder, start);
	_set_sibling(builder, preloop);
}

void fsm_builder_loop_group_end(FsmBuilder *builder)
{
	_join_continuation(builder);
	_reset(builder);
	_pop_frame(builder);
}

void fsm_builder_option_group_start(FsmBuilder *builder)
{
	_ensure_state(builder);

	State *start = builder->state;
	State *cont = malloc(sizeof(State));
	state_init(cont);

	state_add_reference(start, REF_TYPE_DEFAULT, builder->ref_strategy, NULL, cont);

	trace_state("add", cont, "continuation");
	_push_frame(builder, start, cont, start);
}

void fsm_builder_option_group_end(FsmBuilder *builder)
{
	//TODO: if we join the continuations of multiple nested groups we
	// the end state of some groups we will have redundant states.
	// Right now their are connected through empty transitions, but if we
	// start using references (first sets) we may produce memory leaks.
	// The end state of the outer group will not be referenced by any 
	// state, only by the references themselves.
	_join_continuation(builder);
	_pop_frame(builder);
}

void fsm_builder_or(FsmBuilder *builder)
{
	_join_continuation(builder);
	_reset(builder);
}

void fsm_builder_define(FsmBuilder *builder, char *name, int length)
{
	builder->last_nonterminal = fsm_create_nonterminal(builder->fsm, name, length);
	builder->last_symbol = fsm_get_symbol(builder->fsm, name, length);

	//Parent mode set to current builder mode
	builder->last_nonterminal->mode = builder->current_mode;

	_move_to(builder, builder->last_nonterminal->start);
	trace_symbol("set", builder->last_symbol);

	//TODO: implicit fsm_builder_group_start(builder); ??
}

static void _define_mode(FsmBuilder *builder, char *name, int length)
{
	builder->last_nonterminal = fsm_create_nonterminal(builder->fsm, name, length);
	builder->last_symbol = fsm_get_symbol(builder->fsm, name, length);

	//Modes have no parent mode, defaults to mode = 0;

	_move_to(builder, builder->last_nonterminal->start);
	trace_symbol("set mode", builder->last_symbol);
}

void fsm_builder_set_mode(FsmBuilder *builder, char *name, int length)
{
	fsm_create_nonterminal(builder->fsm, name, length);
	Symbol *symbol = fsm_get_symbol(builder->fsm, name, length);
	builder->current_mode = symbol->id;
}

void fsm_builder_mode_push(FsmBuilder *builder, char *name, int length)
{
	fsm_create_nonterminal(builder->fsm, name, length);
	Symbol *symbol = fsm_get_symbol(builder->fsm, name, length);
	builder->last_nonterminal->pushes_mode = symbol->id;
}

void fsm_builder_mode_pop(FsmBuilder *builder)
{
	builder->last_nonterminal->pops_mode = 1;
}

void fsm_builder_identity(FsmBuilder *builder)
{
	if (builder->last_nonterminal) {
		builder->last_nonterminal->type = NONTERMINAL_TYPE_IDENTITY;
	}
}

void fsm_builder_end(FsmBuilder *builder)
{
	if (builder->last_nonterminal) {
		if (builder->last_nonterminal->end) {
			//TODO: Leverage _join_continuation
			if (builder->state) {
				_add_empty(builder, ACTION_EMPTY, NONE);
			}
			_append_state(builder, builder->last_nonterminal->end);
		} else {
			//TODO: implicit fsm_builder_group_end(builder); ??
			//TODO: Move this logic to the fsm frame?
			_ensure_state(builder);
			builder->last_nonterminal->end = builder->state;
			builder->last_nonterminal->sibling_end = builder->sibling;
		}

		// Add proper status to the end state to solve references later
		if(builder->last_nonterminal->status & NONTERMINAL_RETURN_REF) {
			trace_state(
				"end state pending follow-set",
				builder->last_nonterminal->end,
				""
			);
			builder->last_nonterminal->end->status |= STATE_RETURN_REF;
		}
	}
}

void fsm_builder_terminal(FsmBuilder *builder, int symbol)
{
	int type = ACTION_DROP;

	_ensure_state(builder);
	Action *action = state_add(builder->state, symbol, type, NONE);
	_transition(builder, action);
}

void fsm_builder_terminal_range(FsmBuilder *builder, Range range)
{
	int type = ACTION_DROP;

	_ensure_state(builder);
	Action *action = state_add_range(builder->state, range, type, NONE);
	_transition(builder, action);
}

void fsm_builder_any(FsmBuilder *builder)
{
	Range range = {1, 100000};

	fsm_builder_terminal_range(builder, range);
}

/**
 * Creates a reference to a Nonterminal and accepts the associated symbol.
 * Similar to fsm_builder_nonterminal, but accepts instead of dropping and
 * doesn't have a nonterminal references (accepts on empty transition).
 */
void _lexer_nonterminal(FsmBuilder *builder, int symbol_id, bool parallel)
{
	Symbol *sb = fsm_get_symbol_by_id(builder->fsm, symbol_id);
	Nonterminal *nt = fsm_create_nonterminal(builder->fsm, sb->name, sb->length);

	_ensure_state(builder);

	Action *action;
	if(parallel) {
		trace_symbol("Add parallel rule for: ", sb);
		state_add_reference(builder->state, REF_TYPE_COPY_MERGE, builder->ref_strategy, NULL, nt->start);

		//TODO: For now assume end exists, it should use nonterminal refs.
		//TODO: This is not necessary, we can add ACTION_PARTIAL when COPY_MERGE
		_move_to(builder, nt->end);

		action = _add_partial(builder, ACTION_PARTIAL, sb->id);
	} else {
		trace_symbol("Add lexer rule for: ", sb);
		state_add_reference(builder->state, REF_TYPE_START, builder->ref_strategy, NULL, nt->start);

		//TODO: For now assume end exists, it should use nonterminal refs.
		_move_to(builder, nt->end);

		// TODO: Maybe ACTION_ACCEPT should always have a symbol.
		// TODO: Adding accept here makes it difficult to REF_COPY.
		// For now we skip accept actions when deep cloning.
		// Another solution is to clone here instead of modifying the
		// fragments.
		action = _add_empty(builder, ACTION_ACCEPT, sb->id);
	}

	if(nt->type == NONTERMINAL_TYPE_IDENTITY) {
		action->flags |= ACTION_FLAG_IDENTITY;
	}

	// TODO: Add accept to sibling_end?
	// Add mode and flags
	int flags = 0;
	if(nt->pushes_mode) {
		flags |= ACTION_FLAG_MODE_PUSH;
	} else if(nt->pops_mode) {
		flags |= ACTION_FLAG_MODE_POP;
	}

	action->flags |= flags;
	action->mode = nt->pushes_mode;
}

/**
 * Creates a copy reference to a Nonterminal.
 * It creates an inline copy of all states from the nonterminal. 
 */
void fsm_builder_copy(FsmBuilder *builder, char *name, int length)
{
	Nonterminal *nt = fsm_create_nonterminal(builder->fsm, name, length);

	_ensure_state(builder);

	State *from = builder->state;
	State *cont = malloc(sizeof(State));
	state_init(cont);

	state_add_reference_with_cont(from, REF_TYPE_COPY, builder->ref_strategy, NULL, nt->start, nt, cont);

	_move_to(builder, cont);
}

/**
 * Creates a reference to a Nonterminal and shifts the associated symbol.
 */
// TODO: validate error at all locations
int fsm_builder_nonterminal(FsmBuilder *builder, char *name, int length)
{
	Nonterminal *nt = fsm_create_nonterminal(builder->fsm, name, length);
	Symbol *sb = fsm_get_symbol(builder->fsm, name, length);

	_ensure_state(builder);


	State *prev = builder->state;

	Action *action = state_add(builder->state, sb->id, ACTION_POP, NONE);
	_transition(builder, action);

	//Create reference from last non terminal to the named non terminal
	//State exists because we already added the terminal
	state_add_reference(prev, REF_TYPE_SHIFT, builder->ref_strategy, sb, nt->start);

	//Create reference to return from the non terminal to the caller
	//TODO: Should be builder->current->state?
	int error = nonterminal_add_reference(nt, prev, sb);
	check(!error, "Could not add nonterminal ref");
	return 0;
error:
	return -1;
}


static void _set_start(FsmBuilder *builder, int eof_symbol)
{
	Symbol *sb = builder->last_symbol;
	trace_symbol("set initial state", sb);

	_define_mode(builder, nzs(".default"));
	fsm_builder_nonterminal(builder, sb->name, sb->length);

	_ensure_state(builder);
	Action *action = state_add(builder->state, eof_symbol, ACTION_DROP, NONE);

	_transition(builder, action);
	//Is the final accept state necessary? The accept action already 
	//resets to the initial state.
	_ensure_state(builder);

	//TODO: Possible replaces the accept state
	fsm_builder_end(builder);
}

static void _set_lexer_start(FsmBuilder *builder, int eof_symbol, bool parallel)
{
	BMapCursorNonterminal cursor;
	BMapEntryNonterminal *entry;
	Nonterminal *nt;
	State *start;
	bmap_cursor_nonterminal_init(&cursor, &builder->fsm->nonterminals);
	bmap_cursor_nonterminal_revert(&cursor);

	while(bmap_cursor_nonterminal_next(&cursor)) {
		entry = bmap_cursor_nonterminal_current(&cursor);
		nt = entry->nonterminal;
		//Skip modeless nonterminals (they are modes themselves)
		if(!nt->mode) {
			continue;
		}
	
		start = fsm_get_state_by_id(builder->fsm, nt->mode);
		_move_to(builder, start);
		//Different kind of nonterminal reference
		_lexer_nonterminal(builder, entry->key, parallel);
	}
	bmap_cursor_nonterminal_dispose(&cursor);

	_define_mode(builder, nzs(".default"));
	Action *action = state_add(builder->state, eof_symbol, ACTION_ACCEPT, NONE);

	_transition(builder, action);
	//Is the final accept state necessary? The accept action already 
	//resets to the initial state.
	_ensure_state(builder);

	//TODO: Possible replaces the accept state
	fsm_builder_end(builder);
}

void _solve_references(FsmBuilder *builder) {
	BMapCursorNonterminal cursor;
	Nonterminal *nt;
	int some_unsolved;

	//TODO: Are all states reachable through start?
	//TODO: Do all states exist this point and are connected?
	BMapState all_states;

retry:
	bmap_state_init(&all_states);
	state_get_states(fsm_get_state(builder->fsm, nzs(".default")), &all_states);

	some_unsolved = 0;
	bmap_cursor_nonterminal_init(&cursor, &builder->fsm->nonterminals);
	// We try to preserve the definition order (symbols grow negatively)
	bmap_cursor_nonterminal_revert(&cursor);
	while(bmap_cursor_nonterminal_next(&cursor)) {
		nt = bmap_cursor_nonterminal_current(&cursor)->nonterminal;
		/* keys are symbol ids, not strings.
		trace_symbol(
			"solve return references", 
			fsm_get_symbol(builder->fsm, (char *)it.key, it.size)
		);*/

		some_unsolved |= nonterminal_solve_references(nt);

		//TODO: Should avoid collecting states multiple times
		state_get_states(nt->start, &all_states);
	}
	bmap_cursor_nonterminal_dispose(&cursor);

	//Solve return
	State *state;
	BMapCursorState scursor;
	bmap_cursor_state_init(&scursor, &all_states);
	while(bmap_cursor_state_next(&scursor)) {
		state = bmap_cursor_state_current(&scursor)->state;
		some_unsolved |= state_solve_references(state);
	}
	bmap_cursor_state_dispose(&scursor);

	bmap_state_dispose(&all_states);

	if(some_unsolved) {
		//Keep trying until no refs pending.
		//TODO: Detect infinite loops
		goto retry;
	}
}


static void _add_error(FsmBuilder *builder)
{
	Symbol *empty = symbol_table_add(builder->fsm->table, "__empty", 7);
	
	_define_mode(builder, nzs(".error"));
	State *error = builder->state;
	state_add(error, empty->id, ACTION_ERROR, NONE);
	_append_state(builder, error);
}

void fsm_builder_done(FsmBuilder *builder, int eof_symbol) {

	_set_start(builder, eof_symbol);
	_add_error(builder);
	_solve_references(builder);
}

void fsm_builder_lexer_done(FsmBuilder *builder, int eof_symbol) {

	_set_lexer_start(builder, eof_symbol, false);
	_add_error(builder);
	_solve_references(builder);
}

void fsm_builder_parallel_done(FsmBuilder *builder, int eof_symbol) {

	_set_lexer_start(builder, eof_symbol, true);
	_add_error(builder);
	_solve_references(builder);
}
