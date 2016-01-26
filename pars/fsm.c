#include "fsm.h"

#include "cmemory.h"
#include "stack.h"
#include "radixtree.h"
#include "symbols.h"

#define STATE_INIT(V, T, R) (\
		(V).type = (T),\
		(V).reduction = (R)\
	)

#define NONE 0
#include <stdio.h>
#include <stdint.h>

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A) printf("trace: [%p -> %p] %-5s: %-13s (%3i = '%c')\n", T1, T2, M, A, S, (char)S);
#define trace_non_terminal(M, S, L) printf("trace: %-5s: %.*s\n", M, L, S);
#else
#define trace(M, T1, T2, S, A)
#define trace_non_terminal(M, S, L)
#endif

void session_init(Session *session)
{
	session->stack.top = NULL;
	session->index = 0;
	session->handler.context_shift = NULL;
	session->handler.reduce = NULL;
	session->target = NULL;
}

void session_push(Session *session)
{
	SessionNode *node = c_new(SessionNode, 1);
	node->state = session->current;
	node->index = session->index;
	node->next = session->stack.top;
	session->stack.top = node;
}

void session_pop(Session *session)
{
	SessionNode *top = session->stack.top;
	session->current = top->state;
	session->index = top->index;
	session->stack.top = top->next;
	c_delete(top);
}

void session_dispose(Session *session)
{
	while(session->stack.top) {
		session_pop(session);
	}
	//TODO: Instance should not be deleted here
	//Also, it should not be instanced in fsm_start_session
	c_delete(session);
}

void fsm_init(Fsm *fsm)
{
	NODE_INIT(fsm->rules, 0, 0, NULL);
	fsm->symbol_base = -1;
	fsm->start = NULL;
	STATE_INIT(fsm->error, ACTION_TYPE_ERROR, NONE);
	NODE_INIT(fsm->error.next, 0, 0, NULL);
}

void _fsm_get_states(Node *states, State *state)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	//TODO: Should have a separate ptr_to_buffer function
	symbol_to_buffer(buffer, &size, (intptr_t)state);

	State *contained = radix_tree_get(states, buffer, size);

	if(!contained) {
		radix_tree_set(states, buffer, size, state);

		State *st;
		Iterator it;
		radix_tree_iterator_init(&(state->next), &it);
		while((st = (State *)radix_tree_iterator_next(&(state->next), &it)) != NULL) {
			_fsm_get_states(states, st);
		}
		radix_tree_iterator_dispose(&(state->next), &it);
	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_states;
	NonTerminal *nt;
	Iterator it;

	NODE_INIT(all_states, 0, 0, NULL);

	//Get all states reachable through the starting state
	if(fsm->start) {
		_fsm_get_states(&all_states, fsm->start);
	}

	radix_tree_iterator_init(&(fsm->rules), &it);
	while((nt = (NonTerminal *)radix_tree_iterator_next(&(fsm->rules), &it)) != NULL) {
		//Get all states reachable through other rules
		_fsm_get_states(&all_states, nt->start);
		c_delete(nt->name);
		c_delete(nt);
	}
	radix_tree_iterator_dispose(&(fsm->rules), &it);
	radix_tree_dispose(&(fsm->rules));

	//Delete all states
	State *st;
	radix_tree_iterator_init(&all_states, &it);
	while((st = (State *)radix_tree_iterator_next(&all_states, &it)) != NULL) {
		radix_tree_dispose(&st->next);
		c_delete(st);
	}
	radix_tree_iterator_dispose(&all_states, &it);

	radix_tree_dispose(&all_states);
}

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	return radix_tree_get(&fsm->rules, name, length);
}

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start;
}

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm)
{
	cur->fsm = fsm;
	cur->current = NULL;
	cur->stack = NULL;
	cur->followset_stack = NULL;
	cur->last_non_terminal = NULL;
}

void fsm_cursor_dispose(FsmCursor *cur)
{
	stack_dispose(cur->followset_stack);
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
	cursor->current = (State *)cursor->stack->data;
	cursor->stack = stack_pop(cursor->stack);
}

void fsm_cursor_push_followset(FsmCursor *cursor) 
{
	cursor->followset_stack = stack_push(cursor->followset_stack, cursor->current);
}

State *fsm_cursor_pop_followset(FsmCursor *cursor) 
{
	State *state = (State *)cursor->followset_stack->data;
	cursor->followset_stack = stack_pop(cursor->followset_stack);
	return state;
}

void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length)
{
	cur->current = fsm_get_state(cur->fsm, name, length);
}

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length)
{
	NonTerminal *non_terminal = fsm_get_non_terminal(cur->fsm, name, length);
	State *state;
	if(non_terminal == NULL) {
		non_terminal = c_new(NonTerminal, 1);
		state = c_new(State, 1);
		STATE_INIT(*state, ACTION_TYPE_SHIFT, NONE);
		NODE_INIT(state->next, 0, 0, NULL);
		non_terminal->start = state;
		non_terminal->symbol = cur->fsm->symbol_base--;
		non_terminal->length = length;
		non_terminal->name = c_new(char, length); 
		int i = 0;
		for(i; i < length; i++) {
			non_terminal->name[i] = name[i];
		}
		radix_tree_set(&cur->fsm->rules, name, length, non_terminal);
		cur->last_non_terminal = non_terminal;
		//TODO: Add to non_terminal struct: 
		// * other terminals references to be resolved
		// * detect circular references.
	}
	trace_non_terminal("set", name, length);
	cur->current = non_terminal->start;
}

State *_fsm_cursor_add_action_buffer(FsmCursor *cur, unsigned char *buffer, unsigned int size, int action, int reduction, State *state)
{
	if(state == NULL) {
		state = c_new(State, 1);
		STATE_INIT(*state, action, reduction);
		NODE_INIT(state->next, 0, 0, NULL);
		//trace("init", state, action, "");
	}

	//TODO: risk of leak when transitioning using the same symbol
	// on the same state twice.
	radix_tree_set(&cur->current->next, buffer, size, state);
	return state;
}

State *_fsm_cursor_add_action(FsmCursor *cur, int symbol, int action, int reduction, State *state)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, symbol);

	state = _fsm_cursor_add_action_buffer(cur, buffer, size, action, reduction, state);
	trace("add", cur->current, state, symbol, "action");
	return state;
}

State *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length, int symbol)
{
	cur->current = fsm_get_state(cur->fsm, name, length);
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the state.
	cur->current = _fsm_cursor_add_action(cur, symbol, ACTION_TYPE_ACCEPT, NONE, NULL);
}

void fsm_cursor_done(FsmCursor *cur) {
	NonTerminal *nt = cur->last_non_terminal;
	if(nt) {
		fsm_cursor_set_start(cur, nt->name, nt->length, nt->symbol);
	}
}

void fsm_cursor_add_shift(FsmCursor *cur, int symbol)
{
	State *state = _fsm_cursor_add_action(cur, symbol, ACTION_TYPE_SHIFT, NONE, NULL);
	cur->current = state;
}

void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	State *state = _fsm_cursor_add_action(cur, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE, NULL);
	cur->current = state;
}

void fsm_cursor_add_followset(FsmCursor *cur, State *state)
{
	State *s;
	Iterator it;
	radix_tree_iterator_init(&(state->next), &it);
	while((s = (State *)radix_tree_iterator_next(&(state->next), &it)) != NULL) {
		_fsm_cursor_add_action_buffer(cur, it.key, it.size, 0, 0, s);
		trace("add", cur->current, s, buffer_to_symbol(it.key, it.size), "follow");
	}
	radix_tree_iterator_dispose(&(state->next), &it);
}

void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction)
{
	_fsm_cursor_add_action(cur, symbol, ACTION_TYPE_REDUCE, reduction, NULL);
}

Session *fsm_start_session(Fsm *fsm)
{
	Session *session = c_new(Session, 1);
	session->fsm = fsm;
	session->current = fsm->start;
	session->handler.reduce = NULL;
	session->handler.context_shift = NULL;
	session_init(session);
	session_push(session);
	return session;
}

Session *session_set_handler(Session *session, FsmHandler handler, void *target)
{
	session->handler = handler;
	session->target = target;
}

State *session_test(Session *session, int symbol, unsigned int index, unsigned int length)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, symbol);
	State *state;
	State *prev;

	state = radix_tree_get(&session->current->next, buffer, size);
	if(state == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("test", session->current, state, symbol, "error");
			session->current = &session->fsm->error;
		}
		return session->current;
	}

	switch(state->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("test", session->current, state, symbol, "context shift");
		break;
	case ACTION_TYPE_ACCEPT:
		trace("test", session->current, state, symbol, "accept");
		break;
	case ACTION_TYPE_SHIFT:
		trace("test", session->current, state, symbol, "shift");
		break;
	case ACTION_TYPE_REDUCE:
		trace("test", session->current, state, symbol, "reduce");
		break;
	default:
		break;
	}
	return state;
}

void session_match(Session *session, int symbol, unsigned int index, unsigned int length)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	State *state;
	symbol_to_buffer(buffer, &size, symbol);

rematch:
	session->index = index;
	session->length = length;
	state = radix_tree_get(&session->current->next, buffer, size);
	if(state == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("match", session->current, state, symbol, "error");
			session->current = &session->fsm->error;
		}
		return;
	}

	switch(state->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("match", session->current, state, symbol, "context shift");
		if(session->handler.context_shift) {
			session->handler.context_shift(session->target, session->index, session->length, symbol);
		}
		session_push(session);
		session->current = state;
		break;
	case ACTION_TYPE_ACCEPT:
		trace("match", session->current, state, symbol, "accept");
		session->current = state;
		break;
	case ACTION_TYPE_SHIFT:
		trace("match", session->current, state, symbol, "shift");
		session->current = state;
		break;
	case ACTION_TYPE_REDUCE:
		trace("match", session->current, state, symbol, "reduce");
		session_pop(session);
		session->length = index - session->index;
		if(session->handler.reduce) {
			session->handler.reduce(session->target, session->index, session->length, state->reduction);
		}
		session_match(session, state->reduction, session->index, session->length);
		goto rematch; // same as session_match(session, symbol);
		break;
	default:
		break;
	}
}
