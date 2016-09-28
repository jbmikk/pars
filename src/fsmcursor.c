#include "fsmcursor.h"

#include "cmemory.h"
#include "stack.h"

#include <stdio.h>

#define NONE 0

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		T1? ((Action*)T1)->state: NULL, \
		T2, \
		T2? ((Action*)T2)->state: NULL, \
		A, \
		(R != 0)? '>': ' ', \
		R, \
		S, (char)S \
	)
#define trace_state(M, S, A) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s\n", \
		M, NULL, NULL, S, A \
	)
#define trace_non_terminal(M, S, L) printf("trace: %-5s: %.*s\n", M, L, S);
#else
#define trace(M, T1, T2, S, A, R)
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

void fsm_cursor_push(FsmCursor *cursor) 
{
	cursor->stack = stack_push(cursor->stack, cursor->current);
}

void fsm_cursor_pop(FsmCursor *cursor) 
{
	cursor->current = (Action *)cursor->stack->data;
	cursor->stack = stack_pop(cursor->stack);
}

void fsm_cursor_pop_discard(FsmCursor *cursor) 
{
	cursor->stack = stack_pop(cursor->stack);
}

void fsm_cursor_reset(FsmCursor *cursor) 
{
	cursor->current = (Action *)cursor->stack->data;
}

void fsm_cursor_set_end(FsmCursor *cursor) 
{
	//trace("end", cursor->current, 0, 0, "set");
	cursor->last_non_terminal->end = cursor->current;
}

void fsm_cursor_push_continuation(FsmCursor *cursor)
{
	State *state;
	if(cursor->current->state) {
		state = cursor->current->state;
	} else {
		state = c_new(State, 1);
		_state_init(state);
		cursor->current->state = state;
	}

	cursor->continuations = stack_push(cursor->continuations, state);
	trace_state("push", state, "continuation");
}

void fsm_cursor_push_new_continuation(FsmCursor *cursor)
{
	State *state = c_new(State, 1);
	_state_init(state);
	cursor->continuations = stack_push(cursor->continuations, state);
	trace_state("add", state, "continuation");
}

State *fsm_cursor_pop_continuation(FsmCursor *cursor)
{
	State *state = (State *)cursor->continuations->data;
	cursor->continuations = stack_pop(cursor->continuations);
	return state;
}

void fsm_cursor_join_continuation(FsmCursor *cursor)
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
		fsm_cursor_add_empty(cursor);
	}
	cursor->current->state = state;

	trace("add", NULL, cursor->current, 0, "join", 0);
}

void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length)
{
	cur->current = fsm_get_action(cur->fsm, name, length);
}

void fsm_cursor_group_start(FsmCursor *cur)
{
	fsm_cursor_push(cur);
	fsm_cursor_push_new_continuation(cur);
}

void fsm_cursor_loop_group_start(FsmCursor *cur)
{
	fsm_cursor_push_continuation(cur);
	fsm_cursor_push(cur);
}

void fsm_cursor_group_end(FsmCursor *cur)
{
	fsm_cursor_join_continuation(cur);
	fsm_cursor_pop_continuation(cur);
	fsm_cursor_pop(cur);
}

void fsm_cursor_or(FsmCursor *cur)
{
	fsm_cursor_join_continuation(cur);
	fsm_cursor_reset(cur);
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

void fsm_cursor_end(FsmCursor *cur)
{
	//TODO: implicit fsm_cursor_group_end(cur); ??
	fsm_cursor_set_end(cur);
}


/**
 * Creates a reference to a NonTerminal and shifts the associated symbol.
 */
void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length)
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

	fsm_cursor_add_shift(cur, sb->id);
}

Action *_add_action_buffer(Action *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	if(action == NULL) {
		action = c_new(Action, 1);
		_action_init(action, type, reduction, NULL);
	}

	if(!from->state) {
		from->state = c_new(State, 1);
		_state_init(from->state);
	}
	Action *prev = (Action *)radix_tree_try_set(&from->state->actions, buffer, size, action);
	if(prev) {
		if(
			prev->type == action->type &&
			prev->reduction == action->reduction
		) {
			trace("dup", from, action, array_to_int(buffer, size), "skip", reduction);
		} else {
			trace("dup", from, action, array_to_int(buffer, size), "conflict", reduction);
			//TODO: add sentinel ?
		}
		c_delete(action);
		action = NULL;
	}
	return action;
}

Action *_add_action(Action *from, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	_action_init(action, type, reduction, NULL);

	if(!from->state) {
		from->state = c_new(State, 1);
		_state_init(from->state);
	}
	//TODO: detect duplicates
	radix_tree_set_int(&from->state->actions, symbol, action);

	if(type == ACTION_TYPE_CONTEXT_SHIFT) {
		trace("add", from, action, symbol, "context-shift", 0);
	} else if(type == ACTION_TYPE_SHIFT) {
		trace("add", from, action, symbol, "shift", 0);
	} else if(type == ACTION_TYPE_REDUCE) {
		trace("add", from, action, symbol, "reduce", 0);
	} else if(type == ACTION_TYPE_ACCEPT) {
		trace("add", from, action, symbol, "accept", 0);
	} else {
		trace("add", from, action, symbol, "action", 0);
	}
	return action;
}

void _add_first_set(Action *from, State* state)
{
	Action *ac;
	Iterator it;
	radix_tree_iterator_init(&it, &(state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		_add_action_buffer(from, it.key, it.size, 0, 0, ac);
		trace("add", from, ac, array_to_int(it.key, it.size), "first", 0);
	}
	radix_tree_iterator_dispose(&it);
}

void _reduce_follow_set(Action *from, Action *to, int symbol)
{
	Action *ac;
	Iterator it;

	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.
	radix_tree_iterator_init(&it, &(to->state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		_add_action_buffer(from, it.key, it.size, ACTION_TYPE_REDUCE, symbol, NULL);
		trace("add", from, ac, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}

Action *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length)
{
	Symbol *sb = symbol_table_get(cur->fsm->table, name, length);
	NonTerminal *nt = (NonTerminal *)sb->data;
	cur->current = nt->start;
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the action.
	cur->current = _add_action(cur->current, sb->id, ACTION_TYPE_ACCEPT, NONE);

	State *state = c_new(State, 1);
	_state_init(state);
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
	_reduce_follow_set(nt->end, cont, sb->id);
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
	_add_first_set(ref->action, nt->start->state);
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
		_add_action(nt->end, eof_symbol, ACTION_TYPE_REDUCE, sb->id);
		fsm_cursor_set_start(cur, sb->name, sb->length);
	}

	_solve_references(cur);
}

void fsm_cursor_add_shift(FsmCursor *cur, int symbol)
{
	int type;
	if(cur->last_non_terminal->start == cur->current) {
		type = ACTION_TYPE_CONTEXT_SHIFT;
	} else {
		type = ACTION_TYPE_SHIFT;
	}
	Action *action = _add_action(cur->current, symbol, type, NONE);
	cur->last_non_terminal->end = action;
	cur->current = action;
}

void fsm_cursor_add_empty(FsmCursor *cursor)
{
	Fsm *fsm = cursor->fsm;
	Symbol *symbol = symbol_table_get(fsm->table, "__empty", 7);
	Action *action = _add_action(cursor->current, symbol->id, ACTION_TYPE_EMPTY, NONE);
	cursor->last_non_terminal->end = action;
	cursor->current = action;
}

/**
 * TODO: Deprecated by dynamic action shift
 */
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	Action *action = _add_action(cur->current, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE);
	cur->last_non_terminal->end = action;
	cur->current = action;
}

void fsm_cursor_add_first_set(FsmCursor *cur, State *state)
{
	_add_first_set(cur->current, state);
}

void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction)
{
	_add_action(cur->current, symbol, ACTION_TYPE_REDUCE, reduction);
}

