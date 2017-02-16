#include "fsmcursor.h"

#include "cmemory.h"
#include "arrays.h"

#include <stdio.h>
#include <stdint.h>

#define NONE 0

#ifdef FSM_TRACE
#define trace_state(M, S, A) \
	printf( \
		"%-5s: %-9p(refstat:%i) %-13s\n", \
		(M), (S), (S)->status, (A) \
	)
#define trace_symbol(M, S) \
	printf("trace: %-5s: %.*s [id:%i]\n", M, (S)->length, (S)->name, (S)->id);
#else
#define trace_state(M, S, A)
#define trace_symbol(M, S)
#endif

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm)
{
	cur->fsm = fsm;
	cur->action = NULL;
	cur->state = NULL;
	cur->stack = NULL;
	cur->last_symbol = NULL;
	cur->last_nonterminal = NULL;
}

void fsm_cursor_dispose(FsmCursor *cur)
{
	//TODO: In case of interrupted fsm construction some states in the
	// stack should be deleted too.
	//TODO: write test for stack disposal
	while(cur->stack) {
		FsmFrame *frame = cur->stack;
		cur->stack = frame->next;
		c_delete(frame);
	}
	cur->stack = NULL;
	cur->action = NULL;
	cur->state = NULL;
	cur->fsm = NULL;
}

static void _push_frame(FsmCursor *cursor, State *start, State *cont) 
{
	FsmFrame *frame = c_new(FsmFrame, 1);
	frame->start = start;
	frame->continuation = cont;
	frame->next = cursor->stack;
	cursor->stack = frame;
}

static void _pop_frame(FsmCursor *cursor) 
{

	FsmFrame *frame = cursor->stack;
	cursor->stack = frame->next;
	c_delete(frame);
}

static void _append_state(FsmCursor *cursor, State *state)
{
	if(cursor->action) {
		cursor->action->state = state;
	}
	cursor->state = state;
}

static void _move_to(FsmCursor *cursor, State *state)
{
	cursor->action = NULL;
	cursor->state = state;
}

static void _transition(FsmCursor *cursor, Action *action)
{
	cursor->action = action;
	cursor->state = NULL;
}

static void _ensure_state(FsmCursor *cursor)
{
	if(!cursor->state) {
		State *state = c_new(State, 1);
		state_init(state);
		_append_state(cursor, state);
	}
}

static void _add_empty(FsmCursor *cursor)
{
	Fsm *fsm = cursor->fsm;
	Symbol *symbol = symbol_table_get(fsm->table, "__empty", 7);

	_ensure_state(cursor);
	Action *action = state_add(cursor->state, symbol->id, ACTION_EMPTY, NONE);
	_transition(cursor, action);
}

static void _reset(FsmCursor *cursor) 
{
	FsmFrame *frame = cursor->stack;
	_move_to(cursor, frame->start);
}

static void _join_continuation(FsmCursor *cursor)
{
	FsmFrame *frame = cursor->stack;

	if (cursor->state) {
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
		_add_empty(cursor);
		trace_state("add", cursor->state, "empty-action");
	}
	_append_state(cursor, frame->continuation);

	//TODO: Add trace for other types of operations or move to actions?
	//trace("add", NULL, cursor->current, 0, "join", 0);
}

void fsm_cursor_group_start(FsmCursor *cursor)
{
	_ensure_state(cursor);

	State *start = cursor->state;
	State *cont = c_new(State, 1);
	state_init(cont);

	trace_state("add", cont, "continuation");
	_push_frame(cursor, start, cont);
}

void fsm_cursor_group_end(FsmCursor *cursor)
{
	_join_continuation(cursor);
	_pop_frame(cursor);
}

void fsm_cursor_loop_group_start(FsmCursor *cursor)
{
	_ensure_state(cursor);

	State *start = c_new(State, 1);
	state_init(start);

	State *cont = start;

	state_add_reference(cursor->state, NULL, start);

	trace_state("push", cont, "continuation");
	_push_frame(cursor, start, cont);

	_move_to(cursor, start);
}

void fsm_cursor_loop_group_end(FsmCursor *cursor)
{
	_join_continuation(cursor);
	_reset(cursor);
	_pop_frame(cursor);
}

void fsm_cursor_option_group_start(FsmCursor *cursor)
{
	_ensure_state(cursor);

	State *start = cursor->state;
	State *cont = c_new(State, 1);
	state_init(cont);

	state_add_reference(start, NULL, cont);

	trace_state("add", cont, "continuation");
	_push_frame(cursor, start, cont);
}

void fsm_cursor_option_group_end(FsmCursor *cursor)
{
	//TODO: if we join the continuations of multiple nested groups we
	// the end state of some groups we will have redundant states.
	// Right now their are connected through empty transitions, but if we
	// start using references (first sets) we may produce memory leaks.
	// The end state of the outer group will not be referenced by any 
	// state, only by the references themselves.
	_join_continuation(cursor);
	_pop_frame(cursor);
}

void fsm_cursor_or(FsmCursor *cur)
{
	_join_continuation(cur);
	_reset(cur);
}

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length)
{
	Symbol *symbol = fsm_create_nonterminal(cur->fsm, name, length);
	cur->last_symbol = symbol;
	cur->last_nonterminal = (Nonterminal *)symbol->data;
	_move_to(cur, cur->last_nonterminal->start);
	trace_symbol("set", symbol);

	//TODO: implicit fsm_cursor_group_start(cur); ??
}

void fsm_cursor_end(FsmCursor *cursor)
{
	//TODO: implicit fsm_cursor_group_end(cur); ??
	// If that was the case, we wouldn't need to ensure state the exists
	_ensure_state(cursor);
	cursor->last_nonterminal->end = cursor->state;
	//trace("end", cursor->current, 0, 0, "set");

	// Add proper status to the end state to solve references later
	if(cursor->last_nonterminal->status & NONTERMINAL_RETURN_REF) {
		trace_state(
			"end state pending follow-set",
			cursor->last_nonterminal->end,
			""
		);
		cursor->last_nonterminal->end->status |= STATE_RETURN_REF;
	}
}


/**
 * Creates a reference to a Nonterminal and shifts the associated symbol.
 */
void fsm_cursor_nonterminal(FsmCursor *cur, unsigned char *name, int length)
{
	//Get or create symbol and associated non terminal
	Symbol *sb = fsm_create_nonterminal(cur->fsm, name, length);
	Nonterminal *nt = (Nonterminal *)sb->data;

	_ensure_state(cur);

	State *prev = cur->state;

	Action *action = state_add(cur->state, sb->id, ACTION_SHIFT, NONE);
	_transition(cur, action);

	//Create reference from last non terminal to the named non terminal
	//State exists because we already added the terminal
	state_add_reference(prev, sb, nt->start);

	//Create reference to return from the non terminal to the caller
	//TODO: Should be cur->current->state?
	nonterminal_add_reference(nt, prev, sb);
}

static Action *_set_start(FsmCursor *cur, unsigned char *name, int length)
{
	Symbol *sb = symbol_table_get(cur->fsm->table, name, length);
	Nonterminal *nt = (Nonterminal *)sb->data;

	//If start already defined, delete it. Only one start allowed.
	if(cur->fsm->start) {
		//Delete state
		state_dispose(cur->fsm->start);
		c_delete(cur->fsm->start);
	}

	State *initial_state = c_new(State, 1);
	state_init(initial_state);
	cur->fsm->start = initial_state;
	trace_state("add", initial_state, "start state");

	state_add_first_set(cur->fsm->start, nt->start, sb);

	_move_to(cur, cur->fsm->start);

	//TODO: Should check whether current->state is not null?
	//TODO: Is there a test that checks whether this even works?
	if(!radix_tree_contains_int(&cur->state->actions, sb->id)) {
		_ensure_state(cur);
		Action *action = state_add(cur->state, sb->id, ACTION_ACCEPT, NONE);
		_transition(cur, action);
		_append_state(cur, cur->fsm->accept);
	} else {
		//TODO: issue warning or sentinel??
	}
}

int _solve_return_references(FsmCursor *cur, Nonterminal *nt) {
	int unsolved = 0;
        Iterator it;
	Reference *ref;

	if(nt->status == NONTERMINAL_CLEAR) {
		goto end;
	}

	radix_tree_iterator_init(&it, &nt->refs);
	while(ref = (Reference *)radix_tree_iterator_next(&it)) {
		Symbol *sb = ref->symbol;

		if(ref->status == REF_SOLVED) {
			//Ref already solved
			continue;
		}

		Action *cont = radix_tree_get_int(&ref->state->actions, sb->id);

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
			unsolved = 1;
			continue;
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
	radix_tree_iterator_dispose(&it);

	if(!unsolved) {
		nt->status = NONTERMINAL_CLEAR;
		nt->end->status &= ~STATE_RETURN_REF;
		trace_state(
			"state return refs clear",
			nt->end,
			""
		);
	}

end:
	return unsolved;
}

int _solve_invoke_references(FsmCursor *cur, State *state) {
	int unsolved = 0;
        Iterator it;
	Reference *ref;

	if(state->status == STATE_CLEAR) {
		goto end;
	}

	trace_state("solve refs for state", state, "");

	radix_tree_iterator_init(&it, &state->refs);
	while(ref = (Reference *)radix_tree_iterator_next(&it)) {

		if(ref->status == REF_SOLVED) {
			//ref already solved
			continue;
		}

		if(ref->to_state->status != STATE_CLEAR) {
			trace_state(
				"skip first set from",
				ref->to_state,
				""
			);
			unsolved = 1;
			continue;
		}

		//solve reference
		trace_state(
			"append first set from",
			ref->to_state,
			""
		);
		state_add_first_set(ref->state, ref->to_state, ref->symbol);
		ref->status = REF_SOLVED;
	}
	radix_tree_iterator_dispose(&it);

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

void _solve_references(FsmCursor *cur) {
	Node *symbols = &cur->fsm->table->symbols;
        Iterator it;
	Symbol *sb;
	Nonterminal *nt;
	int some_unsolved;

	//TODO: Are all states reachable through start?
	//TODO: Do all states exist this point and are connected?
	Node all_states;
	radix_tree_init(&all_states);
	fsm_get_states(&all_states, cur->fsm->start);

retry:
	some_unsolved = 0;
	radix_tree_iterator_init(&it, symbols);
	while(sb = (Symbol *)radix_tree_iterator_next(&it)) {
		trace_symbol("solve return references from: ", sb);

		nt = (Nonterminal *)sb->data;
		if(nt) {
			some_unsolved |= _solve_return_references(cur, nt);

			//TODO: Should avoid collecting states multiple times
			fsm_get_states(&all_states, nt->start);
		}
	}
	radix_tree_iterator_dispose(&it);

	//Solve return
	State *state;
	radix_tree_iterator_init(&it, &all_states);
	while(state = (State *)radix_tree_iterator_next(&it)) {
		some_unsolved |= _solve_invoke_references(cur, state);
	}
	radix_tree_iterator_dispose(&it);

	if(some_unsolved) {
		//Keep trying until no refs pending.
		//TODO: Detect infinite loops
		goto retry;
	}
	radix_tree_dispose(&all_states);
}

void fsm_cursor_done(FsmCursor *cur, int eof_symbol) {
	Symbol *sb = cur->last_symbol;
	Nonterminal *nt = cur->last_nonterminal;
	if(nt) {
		trace_symbol("main", sb);
		//TODO: Factor out action function to test this
		if(!radix_tree_contains_int(&nt->end->actions, eof_symbol)) {
			_move_to(cur, nt->end);
			state_add(cur->state, eof_symbol, ACTION_REDUCE, sb->id);
		} else {
			//TODO: issue warning or sentinel??
		}
		_solve_references(cur);
		trace_symbol("set initial state", sb);
		_set_start(cur, sb->name, sb->length);
	} else {
		//TODO: issue warning or sentinel??
	}
}

void fsm_cursor_terminal(FsmCursor *cur, int symbol)
{
	int type = ACTION_SHIFT;

	_ensure_state(cur);
	Action *action = state_add(cur->state, symbol, type, NONE);
	_transition(cur, action);
}
