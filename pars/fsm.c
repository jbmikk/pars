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
}

void fsm_init(Fsm *fsm)
{
	radix_tree_init(&fsm->rules, 0, 0, NULL);
	fsm->symbol_base = -1;
	fsm->start = NULL;
	STATE_INIT(fsm->error, ACTION_TYPE_ERROR, NONE);
	radix_tree_init(&fsm->error.next, 0, 0, NULL);
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

	radix_tree_init(&all_states, 0, 0, NULL);

	//Get all states reachable through the starting state
	if(fsm->start) {
		_fsm_get_states(&all_states, fsm->start);
	}

	radix_tree_iterator_init(&(fsm->rules), &it);
	while((nt = (NonTerminal *)radix_tree_iterator_next(&(fsm->rules), &it)) != NULL) {
		//Get all states reachable through other rules
		_fsm_get_states(&all_states, nt->start);
		c_delete(nt->name);
		Reference *ref;
		ref = nt->child_refs;
		while(ref) {
			Reference *cref = ref;
			ref = ref->next;
			c_delete(cref);
		}
		ref = nt->parent_refs;
		while(ref) {
			Reference *pref = ref;
			ref = ref->next;
			c_delete(pref);
		}
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

NonTerminal *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	NonTerminal *non_terminal = fsm_get_non_terminal(fsm, name, length);
	State *state;
	if(non_terminal == NULL) {
		non_terminal = c_new(NonTerminal, 1);
		state = c_new(State, 1);
		STATE_INIT(*state, ACTION_TYPE_SHIFT, NONE);
		radix_tree_init(&state->next, 0, 0, NULL);
		non_terminal->start = state;
		non_terminal->symbol = fsm->symbol_base--;
		non_terminal->length = length;
		non_terminal->name = c_new(char, length); 
		non_terminal->child_refs = NULL;
		non_terminal->parent_refs = NULL;
		int i = 0;
		for(i; i < length; i++) {
			non_terminal->name[i] = name[i];
		}
		radix_tree_set(&fsm->rules, name, length, non_terminal);
		//TODO: Add to non_terminal struct: 
		// * other terminals references to be resolved
		// * detect circular references.
	}
	return non_terminal;
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
	NonTerminal *nt = fsm_create_non_terminal(cur->fsm, name, length);
	cur->last_non_terminal = nt;
	cur->current = nt->start;
	trace_non_terminal("set", name, length);
}

void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length)
{
	NonTerminal *nt = fsm_create_non_terminal(cur->fsm, name, length);

	//Child reference on parent
	Reference *cref = c_new(Reference, 1);
	cref->state = cur->current;
	cref->non_terminal = nt;
	cref->next = cur->last_non_terminal->child_refs;

	cur->last_non_terminal->child_refs = cref;

	//Parent reference on child
	Reference *pref = c_new(Reference, 1);
	pref->state = cur->current;
	pref->non_terminal = cur->last_non_terminal;
	pref->next = nt->parent_refs;

	nt->parent_refs = pref;
}

State *_add_action_buffer(State *from, unsigned char *buffer, unsigned int size, int action, int reduction, State *state)
{
	if(state == NULL) {
		state = c_new(State, 1);
		STATE_INIT(*state, action, reduction);
		radix_tree_init(&state->next, 0, 0, NULL);
		//trace("init", state, action, "");
	}

	//TODO: risk of leak when transitioning using the same symbol
	// on the same state twice.
	radix_tree_set(&from->next, buffer, size, state);
	return state;
}

State *_add_action(State *from, int symbol, int action, int reduction, State *state)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, symbol);

	state = _add_action_buffer(from, buffer, size, action, reduction, state);
	trace("add", from, state, symbol, "action");
	return state;
}

void _add_followset(State *from, State *state)
{
	State *s;
	Iterator it;
	radix_tree_iterator_init(&(state->next), &it);
	while((s = (State *)radix_tree_iterator_next(&(state->next), &it)) != NULL) {
		_add_action_buffer(from, it.key, it.size, 0, 0, s);
		trace("add", from, s, buffer_to_symbol(it.key, it.size), "follow");
	}
	radix_tree_iterator_dispose(&(state->next), &it);
}

State *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length, int symbol)
{
	cur->current = fsm_get_state(cur->fsm, name, length);
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the state.
	cur->current = _add_action(cur->current, symbol, ACTION_TYPE_ACCEPT, NONE, NULL);
}

void fsm_cursor_done(FsmCursor *cur) {
	NonTerminal *nt = cur->last_non_terminal;
	if(nt) {
		fsm_cursor_set_start(cur, nt->name, nt->length, nt->symbol);
	}
}

void fsm_cursor_add_shift(FsmCursor *cur, int symbol)
{
	State *state = _add_action(cur->current, symbol, ACTION_TYPE_SHIFT, NONE, NULL);
	cur->current = state;
}

void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	State *state = _add_action(cur->current, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE, NULL);
	cur->current = state;
}

void fsm_cursor_add_followset(FsmCursor *cur, State *state)
{
	_add_followset(cur->current, state);
}

void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction)
{
	_add_action(cur->current, symbol, ACTION_TYPE_REDUCE, reduction, NULL);
}

Session *session_set_handler(Session *session, FsmHandler handler, void *target)
{
	session->handler = handler;
	session->target = target;
}

State *session_test(Session *session, int symbol, unsigned int index, unsigned int length)
{
	State *state;
	State *prev;

	state = radix_tree_get_int(&session->current->next, symbol);
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
	State *state;

rematch:
	session->index = index;
	session->length = length;
	state = radix_tree_get_int(&session->current->next, symbol);
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
