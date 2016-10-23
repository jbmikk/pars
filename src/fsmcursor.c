#include "fsmcursor.h"

#include "cmemory.h"
#include "stack.h"

#include <stdio.h>

#define NONE 0

#ifdef FSM_TRACE
#define trace_state(M, S, A) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s\n", \
		M, NULL, NULL, S, A \
	)
#define trace_non_terminal(M, S, L) printf("trace: %-5s: %.*s\n", M, L, S);
#else
#define trace_state(M, S, A)
#define trace_non_terminal(M, S, L)
#endif

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm)
{
	cur->fsm = fsm;
	cur->current = NULL;
	cur->stack = NULL;
	cur->continuations = NULL;
	cur->last_symbol = NULL;
	cur->last_non_terminal = NULL;
}

void fsm_cursor_dispose(FsmCursor *cur)
{
	stack_dispose(cur->continuations);
	stack_dispose(cur->stack);
	cur->current = NULL;
	cur->fsm = NULL;
}

static void _reset(FsmCursor *cursor) 
{
	cursor->current = (Action *)cursor->stack->data;
}

static void _stack_push(FsmCursor *cursor) 
{
	cursor->stack = stack_push(cursor->stack, cursor->current);
}

static void _stack_pop(FsmCursor *cursor) 
{
	cursor->stack = stack_pop(cursor->stack);
}

static void _push_continuation(FsmCursor *cursor)
{
	State *state;
	if(cursor->current->state) {
		state = cursor->current->state;
	} else {
		state = c_new(State, 1);
		state_init(state);
		cursor->current->state = state;
	}

	cursor->continuations = stack_push(cursor->continuations, state);
	trace_state("push", state, "continuation");
}

static void _push_new_continuation(FsmCursor *cursor)
{
	State *state = c_new(State, 1);
	state_init(state);
	cursor->continuations = stack_push(cursor->continuations, state);
	trace_state("add", state, "continuation");
}

static State * _pop_continuation(FsmCursor *cursor)
{
	State *state = (State *)cursor->continuations->data;
	cursor->continuations = stack_pop(cursor->continuations);
	return state;
}

static void _add_empty(FsmCursor *cursor)
{
	Fsm *fsm = cursor->fsm;
	Symbol *symbol = symbol_table_get(fsm->table, "__empty", 7);
	Action *action = action_add(cursor->current, symbol->id, ACTION_TYPE_EMPTY, NONE);
	cursor->current = action;
}

static void _join_continuation(FsmCursor *cursor)
{
	State *state = (State *)cursor->continuations->data;

	if (cursor->current->state) {
		// The action is already pointing to a state.
		// We can't point it to another one without losing the current
		// state.
		// The solution is to have the continuation's first set merged
		// into the current state. The continuation is likely not
		// built yet, so we have to either delay the merging until the
		// continuation is ready or add merge them now through an empty
		// transition.
		// For now we add the empty transition.
		_add_empty(cursor);
	}
	cursor->current->state = state;

	//TODO: Add trace for other types of operations or move to actions?
	//trace("add", NULL, cursor->current, 0, "join", 0);
}

void fsm_cursor_group_start(FsmCursor *cur)
{
	_stack_push(cur);
	_push_new_continuation(cur);
}

void fsm_cursor_group_end(FsmCursor *cur)
{
	_join_continuation(cur);
	_pop_continuation(cur);
	_stack_pop(cur);
}

void fsm_cursor_loop_group_start(FsmCursor *cur)
{
	_push_continuation(cur);
	_stack_push(cur);
}

void fsm_cursor_loop_group_end(FsmCursor *cursor)
{
	_join_continuation(cursor);
	_pop_continuation(cursor);
	_reset(cursor);
	_stack_pop(cursor);
}

void fsm_cursor_or(FsmCursor *cur)
{
	_join_continuation(cur);
	_reset(cur);
}

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length)
{
	Symbol *symbol = fsm_create_non_terminal(cur->fsm, name, length);
	cur->last_symbol = symbol;
	cur->last_non_terminal = (NonTerminal *)symbol->data;
	cur->current = cur->last_non_terminal->start;
	trace_non_terminal("set", name, length);

	//TODO: implicit fsm_cursor_group_start(cur); ??
}

void fsm_cursor_end(FsmCursor *cursor)
{
	//TODO: implicit fsm_cursor_group_end(cur); ??
	//trace("end", cursor->current, 0, 0, "set");
	cursor->last_non_terminal->end = cursor->current;
}


/**
 * Creates a reference to a NonTerminal and shifts the associated symbol.
 */
void fsm_cursor_nonterminal(FsmCursor *cur, unsigned char *name, int length)
{
	//Get or create symbol and associated non terminal
	Symbol *sb = fsm_create_non_terminal(cur->fsm, name, length);
	NonTerminal *nt = (NonTerminal *)sb->data;

	//Create reference from last non terminal to the named non terminal
	Reference *pref = c_new(Reference, 1);
	pref->action = cur->current;
	pref->symbol = cur->last_symbol;
	pref->non_terminal = cur->last_non_terminal;
	pref->invoke_status = REF_PENDING;
	pref->return_status = REF_PENDING;

	//Push the reference on the non terminal
	pref->next = nt->parent_refs;
	nt->parent_refs = pref;
	nt->unsolved_returns++;

	//Only count children at the beginning of a non terminal
	if(pref->action == pref->non_terminal->start) {
		pref->non_terminal->unsolved_invokes++;
	}

	fsm_cursor_terminal(cur, sb->id);
}

static Action *_set_start(FsmCursor *cur, unsigned char *name, int length)
{
	Symbol *sb = symbol_table_get(cur->fsm->table, name, length);
	NonTerminal *nt = (NonTerminal *)sb->data;
	cur->current = nt->start;
	cur->fsm->start = cur->current;
	//TODO: calling _set_start multiple times may cause
	// leaks if adding a duplicate accept action to the action.
	int accept = radix_tree_contains_int(&cur->current->state->actions, ACTION_TYPE_ACCEPT);
	if(!accept) {
		cur->current = action_add(cur->current, sb->id, ACTION_TYPE_ACCEPT, NONE);
	} else {
		//TODO: issue warning or sentinel??
	}

	State *state = c_new(State, 1);
	state_init(state);
	//TODO: sentinel if(cur->current->state) ?
	cur->current->state = state;
}

int _solve_return_reference(Symbol *sb, Reference *ref) {
	NonTerminal *nt = (NonTerminal *)sb->data;

	if(ref->return_status == REF_SOLVED) {
		//Ref already solved
		return 0;
	}

	Action *cont = radix_tree_get_int(&ref->action->state->actions, sb->id);
	if(ref->non_terminal->unsolved_returns && ref->non_terminal->end->state == cont->state) {
		trace_non_terminal(
			"skip return ref to",
			ref->symbol->name,
			ref->symbol->length
		);
		return 1;
	}

	//Solve reference
	trace_non_terminal(
		"solve return ref to",
		ref->symbol->name,
		ref->symbol->length
	);
	action_add_reduce_follow_set(nt->end, cont, sb->id);
	ref->return_status = REF_SOLVED;
	nt->unsolved_returns--;
	return 0;
}

int _solve_invoke_reference(Symbol *sb, Reference *ref) {
	NonTerminal *nt = (NonTerminal *)sb->data;

	if(ref->invoke_status == REF_SOLVED) {
		//Ref already solved
		return 0;
	}

	Action *cont = radix_tree_get_int(&ref->action->state->actions, sb->id);
	if(nt->unsolved_invokes) {
		trace_non_terminal(
			"skip invoke ref from",
			ref->symbol->name,
			ref->symbol->length
		);
		return 1;
	}

	//Solve reference
	trace_non_terminal(
		"solve invoke ref from",
		ref->symbol->name,
		ref->symbol->length
	);
	action_add_first_set(ref->action, nt->start->state);
	ref->invoke_status = REF_SOLVED;
	if(ref->action == ref->non_terminal->start) {
		ref->non_terminal->unsolved_invokes--;
	}
	return 0;
}

void _solve_references(FsmCursor *cur) {
	Node *symbols = &cur->fsm->table->symbols;
	Iterator it;
	Symbol *sb;
	NonTerminal *nt;
	int some_unsolved;
retry:
	some_unsolved = 0;
	radix_tree_iterator_init(&it, symbols);
	while(sb = (Symbol *)radix_tree_iterator_next(&it)) {
		trace_non_terminal("solve references", sb->name, sb->length);

		nt = (NonTerminal *)sb->data;
		if(nt) {
			Reference *ref;
			ref = nt->parent_refs;
			while(ref) {
				some_unsolved |= _solve_return_reference(sb, ref);
				some_unsolved |= _solve_invoke_reference(sb, ref);
				ref = ref->next;
			}
		}
	}
	radix_tree_iterator_dispose(&it);
	if(some_unsolved) {
		//Keep trying until no refs pending.
		//TODO: Detect infinite loops
		goto retry;
	}
}

void fsm_cursor_done(FsmCursor *cur, int eof_symbol) {
	Symbol *sb = cur->last_symbol;
	NonTerminal *nt = cur->last_non_terminal;
	if(nt) {
		//TODO: May cause leaks if L_EOF previously added
		trace_non_terminal("main", sb->name, sb->length);
		action_add(nt->end, eof_symbol, ACTION_TYPE_REDUCE, sb->id);
		_set_start(cur, sb->name, sb->length);
	}

	_solve_references(cur);
}

void fsm_cursor_terminal(FsmCursor *cur, int symbol)
{
	int type;
	if(cur->last_non_terminal->start == cur->current) {
		type = ACTION_TYPE_CONTEXT_SHIFT;
	} else {
		type = ACTION_TYPE_SHIFT;
	}
	Action *action = action_add(cur->current, symbol, type, NONE);
	cur->current = action;
}
