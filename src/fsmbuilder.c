#include "fsmbuilder.h"

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

void fsm_builder_init(FsmBuilder *builder, Fsm *fsm)
{
	builder->fsm = fsm;
	builder->action = NULL;
	builder->state = NULL;
	builder->stack = NULL;
	builder->last_symbol = NULL;
	builder->last_nonterminal = NULL;
}

void fsm_builder_dispose(FsmBuilder *builder)
{
	//TODO: In case of interrupted fsm construction some states in the
	// stack should be deleted too.
	//TODO: write test for stack disposal
	while(builder->stack) {
		FsmFrame *frame = builder->stack;
		builder->stack = frame->next;
		c_delete(frame);
	}
	builder->stack = NULL;
	builder->action = NULL;
	builder->state = NULL;
	builder->fsm = NULL;
}

static void _push_frame(FsmBuilder *builder, State *start, State *cont) 
{
	FsmFrame *frame = c_new(FsmFrame, 1);
	frame->start = start;
	frame->continuation = cont;
	frame->next = builder->stack;
	builder->stack = frame;
}

static void _pop_frame(FsmBuilder *builder) 
{

	FsmFrame *frame = builder->stack;
	builder->stack = frame->next;
	c_delete(frame);
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

static void _transition(FsmBuilder *builder, Action *action)
{
	builder->action = action;
	builder->state = NULL;
}

static void _ensure_state(FsmBuilder *builder)
{
	if(!builder->state) {
		State *state = c_new(State, 1);
		state_init(state);
		_append_state(builder, state);
	}
}

static void _add_empty(FsmBuilder *builder)
{
	Fsm *fsm = builder->fsm;
	Symbol *symbol = symbol_table_get(fsm->table, "__empty", 7);

	_ensure_state(builder);
	Action *action = state_add(builder->state, symbol->id, ACTION_EMPTY, NONE);
	_transition(builder, action);
}

static void _reset(FsmBuilder *builder) 
{
	FsmFrame *frame = builder->stack;
	_move_to(builder, frame->start);
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
		_add_empty(builder);
		trace_state("add", builder->state, "empty-action");
	}
	_append_state(builder, frame->continuation);

	//TODO: Add trace for other types of operations or move to actions?
	//trace("add", NULL, builder->current, 0, "join", 0);
}

void fsm_builder_group_start(FsmBuilder *builder)
{
	_ensure_state(builder);

	State *start = builder->state;
	State *cont = c_new(State, 1);
	state_init(cont);

	trace_state("add", cont, "continuation");
	_push_frame(builder, start, cont);
}

void fsm_builder_group_end(FsmBuilder *builder)
{
	_join_continuation(builder);
	_pop_frame(builder);
}

void fsm_builder_loop_group_start(FsmBuilder *builder)
{
	_ensure_state(builder);

	State *start = c_new(State, 1);
	state_init(start);

	State *cont = start;

	state_add_reference(builder->state, NULL, start);

	trace_state("push", cont, "continuation");
	_push_frame(builder, start, cont);

	_move_to(builder, start);
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
	State *cont = c_new(State, 1);
	state_init(cont);

	state_add_reference(start, NULL, cont);

	trace_state("add", cont, "continuation");
	_push_frame(builder, start, cont);
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
	Symbol *symbol = fsm_create_nonterminal(builder->fsm, name, length);
	builder->last_symbol = symbol;
	builder->last_nonterminal = (Nonterminal *)symbol->data;
	_move_to(builder, builder->last_nonterminal->start);
	trace_symbol("set", symbol);

	//TODO: implicit fsm_builder_group_start(builder); ??
}

void fsm_builder_end(FsmBuilder *builder)
{
	//TODO: implicit fsm_builder_group_end(builder); ??
	// If that was the case, we wouldn't need to ensure state the exists
	_ensure_state(builder);
	builder->last_nonterminal->end = builder->state;
	//trace("end", builder->current, 0, 0, "set");

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

void fsm_builder_terminal(FsmBuilder *builder, int symbol)
{
	int type = ACTION_DROP;

	_ensure_state(builder);
	Action *action = state_add(builder->state, symbol, type, NONE);
	_transition(builder, action);
}

/**
 * Creates a reference to a Nonterminal and shifts the associated symbol.
 */
void fsm_builder_nonterminal(FsmBuilder *builder, char *name, int length)
{
	//Get or create symbol and associated non terminal
	Symbol *sb = fsm_create_nonterminal(builder->fsm, name, length);
	Nonterminal *nt = (Nonterminal *)sb->data;

	_ensure_state(builder);

	State *prev = builder->state;

	Action *action = state_add(builder->state, sb->id, ACTION_DROP, NONE);
	_transition(builder, action);

	//Create reference from last non terminal to the named non terminal
	//State exists because we already added the terminal
	state_add_reference(prev, sb, nt->start);

	//Create reference to return from the non terminal to the caller
	//TODO: Should be builder->current->state?
	nonterminal_add_reference(nt, prev, sb);
}

static void _set_start(FsmBuilder *builder, char *name, int length)
{
	Symbol *sb = symbol_table_get(builder->fsm->table, name, length);
	Nonterminal *nt = (Nonterminal *)sb->data;

	//If start already defined, delete it. Only one start allowed.
	if(builder->fsm->start) {
		//Delete state
		state_dispose(builder->fsm->start);
		c_delete(builder->fsm->start);
	}

	State *initial_state = c_new(State, 1);
	state_init(initial_state);
	builder->fsm->start = initial_state;
	trace_state("add", initial_state, "start state");

	state_add_first_set(builder->fsm->start, nt->start, sb);

	_move_to(builder, builder->fsm->start);

	//TODO: Should check whether current->state is not null?
	//TODO: Is there a test that checks whether this even works?
	if(!radix_tree_contains_int(&builder->state->actions, sb->id)) {
		_ensure_state(builder);
		Action *action = state_add(builder->state, sb->id, ACTION_ACCEPT, NONE);
		_transition(builder, action);
		_append_state(builder, builder->fsm->accept);
	} else {
		//TODO: issue warning or sentinel??
	}
}

int _solve_return_references(FsmBuilder *builder, Nonterminal *nt) {
	int unsolved = 0;
        Iterator it;
	Reference *ref;

	if(nt->status == NONTERMINAL_CLEAR) {
		goto end;
	}

	radix_tree_iterator_init(&it, &nt->refs);
	while((ref = (Reference *)radix_tree_iterator_next(&it))) {
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

int _solve_invoke_references(FsmBuilder *builder, State *state) {
	int unsolved = 0;
        Iterator it;
	Reference *ref;

	if(state->status == STATE_CLEAR) {
		goto end;
	}

	trace_state("solve refs for state", state, "");

	radix_tree_iterator_init(&it, &state->refs);
	while((ref = (Reference *)radix_tree_iterator_next(&it))) {

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

void _solve_references(FsmBuilder *builder) {
	Node *symbols = &builder->fsm->table->symbols;
        Iterator it;
	Symbol *sb;
	Nonterminal *nt;
	int some_unsolved;

	//TODO: Are all states reachable through start?
	//TODO: Do all states exist this point and are connected?
	Node all_states;
	radix_tree_init(&all_states);
	fsm_get_states(&all_states, builder->fsm->start);

retry:
	some_unsolved = 0;
	radix_tree_iterator_init(&it, symbols);
	while((sb = (Symbol *)radix_tree_iterator_next(&it))) {
		trace_symbol("solve return references from: ", sb);

		nt = (Nonterminal *)sb->data;
		if(nt) {
			some_unsolved |= _solve_return_references(builder, nt);

			//TODO: Should avoid collecting states multiple times
			fsm_get_states(&all_states, nt->start);
		}
	}
	radix_tree_iterator_dispose(&it);

	//Solve return
	State *state;
	radix_tree_iterator_init(&it, &all_states);
	while((state = (State *)radix_tree_iterator_next(&it))) {
		some_unsolved |= _solve_invoke_references(builder, state);
	}
	radix_tree_iterator_dispose(&it);

	if(some_unsolved) {
		//Keep trying until no refs pending.
		//TODO: Detect infinite loops
		goto retry;
	}
	radix_tree_dispose(&all_states);
}

void fsm_builder_done(FsmBuilder *builder, int eof_symbol) {
	Symbol *sb = builder->last_symbol;
	Nonterminal *nt = builder->last_nonterminal;
	if(nt) {
		trace_symbol("main", sb);
		//TODO: Factor out action function to test this
		if(!radix_tree_contains_int(&nt->end->actions, eof_symbol)) {
			_move_to(builder, nt->end);
			state_add(builder->state, eof_symbol, ACTION_REDUCE, sb->id);
		} else {
			//TODO: issue warning or sentinel??
		}
		_solve_references(builder);
		trace_symbol("set initial state", sb);
		_set_start(builder, sb->name, sb->length);
	} else {
		//TODO: issue warning or sentinel??
	}
}