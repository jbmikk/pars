#include "fsm.h"

#include "cmemory.h"
#include "stack.h"
#include "radixtree.h"
#include "symbols.h"
#include "radixtree_ext.h"

#define NONE 0
#include <stdio.h>
#include <stdint.h>

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

void _state_init(State *state)
{
	radix_tree_init(&state->actions, 0, 0, NULL);
}

void _action_init(Action *action, char type, int reduction, State *state)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
}

void session_init(Session *session, Fsm *fsm)
{
	session->fsm = fsm;
	session->current = fsm->start;
	session->stack.top = NULL;
	session->index = 0;
	session->handler.context_shift = NULL;
	session->handler.reduce = NULL;
	session->target = NULL;
	//TODO: Avoid calling push in init
	session_push(session);
}

void session_push(Session *session)
{
	SessionNode *node = c_new(SessionNode, 1);
	node->action = session->current;
	node->index = session->index;
	node->next = session->stack.top;
	session->stack.top = node;
}

void session_pop(Session *session)
{
	SessionNode *top = session->stack.top;
	session->current = top->action;
	session->index = top->index;
	session->stack.top = top->next;
	c_delete(top);
}

void session_dispose(Session *session)
{
	while(session->stack.top) {
		session_pop(session);
	}
}

void fsm_init(Fsm *fsm)
{
	radix_tree_init(&fsm->rules, 0, 0, NULL);
	fsm->symbol_base = -1;
	fsm->start = NULL;
	_action_init(&fsm->error, ACTION_TYPE_ERROR, NONE, NULL);
	fsm->error.state = c_new(State, 1);
	_state_init(fsm->error.state);
}

void _fsm_get_actions(Node *actions, Node *states, Action *action)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	//TODO: Should have a separate ptr_to_buffer function
	symbol_to_buffer(buffer, &size, (intptr_t)action);

	Action *in_actions = radix_tree_get(actions, buffer, size);

	if(!in_actions) {
		radix_tree_set(actions, buffer, size, action);

		State *state = action->state;
		if(state) {
			Action *ac;
			Iterator it;
			radix_tree_iterator_init(&it, &(state->actions));
			while(ac = (Action *)radix_tree_iterator_next(&it)) {
				_fsm_get_actions(actions, states, ac);
			}
			radix_tree_iterator_dispose(&it);

			//Add state to states
			symbol_to_buffer(buffer, &size, (intptr_t)state);
			State *in_states = radix_tree_get(states, buffer, size);
			if(!in_states) {
				radix_tree_set(states, buffer, size, state);
			}
		}
	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_actions;
	Node all_states;
	NonTerminal *nt;
	Iterator it;

	radix_tree_init(&all_actions, 0, 0, NULL);
	radix_tree_init(&all_states, 0, 0, NULL);

	//Get all actions reachable through the starting action
	if(fsm->start) {
		_fsm_get_actions(&all_actions, &all_states, fsm->start);
	}

	radix_tree_iterator_init(&it, &(fsm->rules));
	while(nt = (NonTerminal *)radix_tree_iterator_next(&it)) {
		//Get all actions reachable through other rules
		_fsm_get_actions(&all_actions, &all_states, nt->start);
		c_delete(nt->name);
		Reference *ref = nt->parent_refs;
		while(ref) {
			Reference *pref = ref;
			ref = ref->next;
			c_delete(pref);
		}
		c_delete(nt);
	}
	radix_tree_iterator_dispose(&it);
	radix_tree_dispose(&(fsm->rules));

	//Delete all actions
	Action *ac;
	radix_tree_iterator_init(&it, &all_actions);
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		c_delete(ac);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&all_actions);

	//Delete all states
	State *st;
	radix_tree_iterator_init(&it, &all_states);
	while(st = (State *)radix_tree_iterator_next(&it)) {
		radix_tree_dispose(&st->actions);
		c_delete(st);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&all_states);

	//Delete error state
	radix_tree_dispose(&fsm->error.state->actions);
	c_delete(fsm->error.state);
}

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	return radix_tree_get(&fsm->rules, name, length);
}

NonTerminal *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	NonTerminal *non_terminal = fsm_get_non_terminal(fsm, name, length);
	Action *action;
	if(non_terminal == NULL) {
		non_terminal = c_new(NonTerminal, 1);
		action = c_new(Action, 1);
		_action_init(action, ACTION_TYPE_SHIFT, NONE, NULL);
		non_terminal->start = action;
		non_terminal->end = action;
		non_terminal->symbol = fsm->symbol_base--;
		non_terminal->length = length;
		non_terminal->name = c_new(char, length); 
		non_terminal->parent_refs = NULL;
		non_terminal->unsolved_returns = 0;
		non_terminal->unsolved_invokes = 0;
		int i = 0;
		for(i; i < length; i++) {
			non_terminal->name[i] = name[i];
		}
		radix_tree_set(&fsm->rules, name, length, non_terminal);
		//TODO: Add to non_terminal struct: 
		// * detect circular references.
	}
	return non_terminal;
}


Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start;
}

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start->state;
}

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm)
{
	cur->fsm = fsm;
	cur->current = NULL;
	cur->stack = NULL;
	cur->continuations = NULL;
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

	//TODO: sentinel if(cursor->current->state) ?
	cursor->current->state = state;

	trace("add", NULL, cursor->current, 0, "join", 0);
}

void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length)
{
	cur->current = fsm_get_action(cur->fsm, name, length);
}

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length)
{
	NonTerminal *nt = fsm_create_non_terminal(cur->fsm, name, length);
	cur->last_non_terminal = nt;
	cur->current = nt->start;
	trace_non_terminal("set", name, length);
}

void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length)
{
	NonTerminal *nt = fsm_create_non_terminal(cur->fsm, name, length);

	//Parent reference on child
	Reference *pref = c_new(Reference, 1);
	pref->action = cur->current;
	pref->non_terminal = cur->last_non_terminal;
	pref->next = nt->parent_refs;
	pref->invoke_status = REF_PENDING;
	pref->return_status = REF_PENDING;

	nt->parent_refs = pref;
	nt->unsolved_returns++;

	//Only count children at the beginning of a non terminal
	if(pref->action == pref->non_terminal->start) {
		pref->non_terminal->unsolved_invokes++;
	}

	fsm_cursor_add_shift(cur, nt->symbol);
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
			trace("dup", from, action, buffer_to_symbol(buffer, size), "skip", reduction);
		} else {
			trace("dup", from, action, buffer_to_symbol(buffer, size), "conflict", reduction);
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

void _add_followset(Action *from, State* state)
{
	Action *ac;
	Iterator it;
	radix_tree_iterator_init(&it, &(state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		_add_action_buffer(from, it.key, it.size, 0, 0, ac);
		trace("add", from, ac, buffer_to_symbol(it.key, it.size), "follow", 0);
	}
	radix_tree_iterator_dispose(&it);
}

void _reduce_followset(Action *from, Action *to, int symbol)
{
	Action *ac;
	Iterator it;
	radix_tree_iterator_init(&it, &(to->state->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		_add_action_buffer(from, it.key, it.size, ACTION_TYPE_REDUCE, symbol, NULL);
		trace("add", from, ac, buffer_to_symbol(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}

Action *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length, int symbol)
{
	cur->current = fsm_get_action(cur->fsm, name, length);
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the action.
	cur->current = _add_action(cur->current, symbol, ACTION_TYPE_ACCEPT, NONE);

	State *state = c_new(State, 1);
	_state_init(state);
	//TODO: sentinel if(cur->current->state) ?
	cur->current->state = state;
}

int _solve_return_reference(NonTerminal *nt, Reference *ref) {
	if(ref->return_status == REF_SOLVED) {
		//Ref already solved
		return 0;
	}

	Action *cont = radix_tree_get_int(&ref->action->state->actions, nt->symbol);
	if(ref->non_terminal->unsolved_returns && ref->non_terminal->end->state == cont->state) {
		trace_non_terminal(
			"skip return ref to",
			ref->non_terminal->name,
			ref->non_terminal->length
		);
		return 1;
	}

	//Solve reference
	trace_non_terminal(
		"solve return ref to",
		ref->non_terminal->name,
		ref->non_terminal->length
	);
	_reduce_followset(nt->end, cont, nt->symbol);
	ref->return_status = REF_SOLVED;
	nt->unsolved_returns--;
	return 0;
}

int _solve_invoke_reference(NonTerminal *nt, Reference *ref) {
	if(ref->invoke_status == REF_SOLVED) {
		//Ref already solved
		return 0;
	}

	Action *cont = radix_tree_get_int(&ref->action->state->actions, nt->symbol);
	if(nt->unsolved_invokes) {
		trace_non_terminal(
			"skip invoke ref from",
			ref->non_terminal->name,
			ref->non_terminal->length
		);
		return 1;
	}

	//Solve reference
	trace_non_terminal(
		"solve invoke ref from",
		ref->non_terminal->name,
		ref->non_terminal->length
	);
	_add_followset(ref->action, nt->start->state);
	ref->invoke_status = REF_SOLVED;
	if(ref->action == ref->non_terminal->start) {
		ref->non_terminal->unsolved_invokes--;
	}
	return 0;
}

void _solve_references(FsmCursor *cur) {
	Node * rules = &cur->fsm->rules;
	Iterator it;
	NonTerminal *nt;
	int some_unsolved;
retry:
	some_unsolved = 0;
	radix_tree_iterator_init(&it, rules);
	while(nt = (NonTerminal *)radix_tree_iterator_next(&it)) {
		trace_non_terminal("solve references", nt->name, nt->length);
		Reference *ref;
		ref = nt->parent_refs;
		while(ref) {
			some_unsolved |= _solve_return_reference(nt, ref);
			some_unsolved |= _solve_invoke_reference(nt, ref);
			ref = ref->next;
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
	NonTerminal *nt = cur->last_non_terminal;
	if(nt) {
		//TODO: May cause leaks if L_EOF previously added
		trace_non_terminal("main", nt->name, nt->length);
		_add_action(nt->end, eof_symbol, ACTION_TYPE_REDUCE, nt->symbol);
		fsm_cursor_set_start(cur, nt->name, nt->length, nt->symbol);
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

/**
 * TODO: Deprecated by dynamic action shift
 */
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	Action *action = _add_action(cur->current, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE);
	cur->last_non_terminal->end = action;
	cur->current = action;
}

void fsm_cursor_add_followset(FsmCursor *cur, State *state)
{
	_add_followset(cur->current, state);
}

void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction)
{
	_add_action(cur->current, symbol, ACTION_TYPE_REDUCE, reduction);
}

Session *session_set_handler(Session *session, FsmHandler handler, void *target)
{
	session->handler = handler;
	session->target = target;
}

Action *session_test(Session *session, int symbol, unsigned int index, unsigned int length)
{
	Action *action;
	Action *prev;

	action = radix_tree_get_int(&session->current->state->actions, symbol);
	if(action == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("test", session->current, action, symbol, "error", 0);
			session->current = &session->fsm->error;
		}
		return session->current;
	}

	switch(action->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("test", session->current, action, symbol, "context shift", 0);
		break;
	case ACTION_TYPE_ACCEPT:
		trace("test", session->current, action, symbol, "accept", 0);
		break;
	case ACTION_TYPE_SHIFT:
		trace("test", session->current, action, symbol, "shift", 0);
		break;
	case ACTION_TYPE_REDUCE:
		trace("test", session->current, action, symbol, "reduce", action->reduction);
		break;
	default:
		break;
	}
	return action;
}

void session_match(Session *session, int symbol, unsigned int index, unsigned int length)
{
	Action *action;

rematch:
	session->index = index;
	session->length = length;
	action = radix_tree_get_int(&session->current->state->actions, symbol);
	if(action == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("match", session->current, action, symbol, "error", 0);
			session->current = &session->fsm->error;
		}
		return;
	}

	switch(action->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("match", session->current, action, symbol, "context shift", 0);
		if(session->handler.context_shift) {
			session->handler.context_shift(session->target, session->index, session->length, symbol);
		}
		session_push(session);
		session->current = action;
		break;
	case ACTION_TYPE_ACCEPT:
		trace("match", session->current, action, symbol, "accept", 0);
		session->current = action;
		break;
	case ACTION_TYPE_SHIFT:
		trace("match", session->current, action, symbol, "shift", 0);
		session->current = action;
		break;
	case ACTION_TYPE_REDUCE:
		trace("match", session->current, action, symbol, "reduce", action->reduction);
		session_pop(session);
		session->length = index - session->index;
		if(session->handler.reduce) {
			session->handler.reduce(session->target, session->index, session->length, action->reduction);
		}
		session_match(session, action->reduction, session->index, session->length);
		goto rematch; // same as session_match(session, symbol);
		break;
	default:
		break;
	}
}
