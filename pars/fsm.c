#include "fsm.h"

#include "cmemory.h"
#include "stack.h"
#include "radixtree.h"
#include "symbols.h"

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

void _state_init(State *state, char action, int reduction)
{
	state->type = action;
	state->reduction = reduction;
	radix_tree_init(&state->next, 0, 0, NULL);
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
	_state_init(&fsm->error, ACTION_TYPE_ERROR, NONE);
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
		_state_init(state, ACTION_TYPE_SHIFT, NONE);
		non_terminal->start = state;
		non_terminal->end = state;
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
	cursor->current = (State *)cursor->stack->data;
	cursor->previous = NULL;
	cursor->stack = stack_pop(cursor->stack);
}

void fsm_cursor_reset(FsmCursor *cursor) 
{
	cursor->current = (State *)cursor->stack->data;
	cursor->previous = NULL;
}

void fsm_cursor_push_continuation(FsmCursor *cursor)
{
	cursor->continuations = stack_push(cursor->continuations, cursor->current);
}

void fsm_cursor_push_new_continuation(FsmCursor *cursor)
{
	State *state = c_new(State, 1);
	_state_init(state, ACTION_TYPE_SHIFT, NONE);
	cursor->continuations = stack_push(cursor->continuations, state);
	trace("add", state, NULL, 0, "continuation");
}

State *fsm_cursor_pop_continuation(FsmCursor *cursor)
{
	State *state = (State *)cursor->continuations->data;
	cursor->continuations = stack_pop(cursor->continuations);
	return state;
}

void fsm_cursor_join_continuation(FsmCursor *cursor)
{
	State *cont = (State *)cursor->continuations->data;
	int symbol = cursor->last_symbol;

	trace("add", cursor->previous, cont, symbol, "join");

	State *old = radix_tree_get_int(&cursor->previous->next, symbol);
	//TODO: Ugly hack. This will not work for mixed branches.
	//Branches with a single transition mixed with branches with
	//multiple transitions will have different actions (shift + cs);
	cont->type = old->type;

	radix_tree_set_int(&cursor->previous->next, symbol, cont);
	radix_tree_dispose(&old->next);
	c_delete(old);

	cursor->current = cont;
	cursor->last_non_terminal->end = cont;
}

void fsm_cursor_follow_continuation(FsmCursor *cursor)
{
	cursor->current = fsm_cursor_pop_continuation(cursor);
}

void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length)
{
	cur->current = fsm_get_state(cur->fsm, name, length);
	cur->previous = NULL;
}

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length)
{
	NonTerminal *nt = fsm_create_non_terminal(cur->fsm, name, length);
	cur->last_non_terminal = nt;
	cur->current = nt->start;
	cur->previous = NULL;
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
		_state_init(state, action, reduction);
	}

	//TODO: risk of leak when transitioning using the same symbol
	// on the same state twice.
	radix_tree_set(&from->next, buffer, size, state);
	return state;
}

State *_add_action(State *from, int symbol, int action, int reduction)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, symbol);

	State * state = c_new(State, 1);
	_state_init(state, action, reduction);
	//TODO: risk of leak when transitioning using the same symbol
	// on the same state twice.
	radix_tree_set(&from->next, buffer, size, state);

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

void _reduce_followset(State *from, State *to, int symbol)
{
	State *s;
	Iterator it;
	radix_tree_iterator_init(&(to->next), &it);
	while((s = (State *)radix_tree_iterator_next(&(to->next), &it)) != NULL) {
		_add_action_buffer(from, it.key, it.size, ACTION_TYPE_REDUCE, symbol, NULL);
		trace("add", from, s, buffer_to_symbol(it.key, it.size), "reduce-follow");
	}
	radix_tree_iterator_dispose(&(to->next), &it);
}

State *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length, int symbol)
{
	cur->current = fsm_get_state(cur->fsm, name, length);
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the state.
	cur->current = _add_action(cur->current, symbol, ACTION_TYPE_ACCEPT, NONE);
	//TODO: cur->previous = NULL;
}

void fsm_cursor_done(FsmCursor *cur, int eof_symbol) {
	NonTerminal *nt = cur->last_non_terminal;
	if(nt) {
		//TODO: May cause leaks if L_EOF previously added
		trace_non_terminal("main", nt->name, nt->length);
		_add_action(nt->end, eof_symbol, ACTION_TYPE_REDUCE, nt->symbol);
		fsm_cursor_set_start(cur, nt->name, nt->length, nt->symbol);
	}
	
	//Solve child and parent references
	Node * rules = &cur->fsm->rules;

	Iterator it;
	radix_tree_iterator_init(rules, &it);
	while(nt = (NonTerminal *)radix_tree_iterator_next(rules, &it)) {
		Reference *ref;
		ref = nt->child_refs;
		trace_non_terminal("solve", nt->name, nt->length);
		while(ref) {
			_add_followset(ref->state, ref->non_terminal->start);
			ref = ref->next;
		}

		ref = nt->parent_refs;
		while(ref) {
			State *cont = radix_tree_get_int(&ref->state->next, nt->symbol);
			_reduce_followset(nt->end, cont, nt->symbol);
			ref = ref->next;
		}
	}
	radix_tree_iterator_dispose(rules, &it);
}

void fsm_cursor_add_shift(FsmCursor *cur, int symbol)
{
	int action;
	if(cur->last_non_terminal->start == cur->current) {
		action = ACTION_TYPE_CONTEXT_SHIFT;
	} else {
		action = ACTION_TYPE_SHIFT;
	}
	State *state = _add_action(cur->current, symbol, action, NONE);
	cur->last_non_terminal->end = state;
	cur->previous = cur->current;
	cur->last_symbol = symbol;
	cur->current = state;
}

/**
 * TODO: Deprecated by dynamic action shift
 */
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	State *state = _add_action(cur->current, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE);
	cur->last_non_terminal->end = state;
	cur->previous = cur->current;
	cur->last_symbol = symbol;
	cur->current = state;
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
