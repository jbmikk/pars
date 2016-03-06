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

void _action_init(Action *action, char type, int reduction)
{
	action->type = type;
	action->reduction = reduction;
	radix_tree_init(&action->next, 0, 0, NULL);
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
	_action_init(&fsm->error, ACTION_TYPE_ERROR, NONE);
}

void _fsm_get_actions(Node *actions, Action *action)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	//TODO: Should have a separate ptr_to_buffer function
	symbol_to_buffer(buffer, &size, (intptr_t)action);

	Action *contained = radix_tree_get(actions, buffer, size);

	if(!contained) {
		radix_tree_set(actions, buffer, size, action);

		Action *st;
		Iterator it;
		radix_tree_iterator_init(&(action->next), &it);
		while(st = (Action *)radix_tree_iterator_next(&(action->next), &it)) {
			_fsm_get_actions(actions, st);
		}
		radix_tree_iterator_dispose(&(action->next), &it);
	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_actions;
	NonTerminal *nt;
	Iterator it;

	radix_tree_init(&all_actions, 0, 0, NULL);

	//Get all actions reachable through the starting action
	if(fsm->start) {
		_fsm_get_actions(&all_actions, fsm->start);
	}

	radix_tree_iterator_init(&(fsm->rules), &it);
	while(nt = (NonTerminal *)radix_tree_iterator_next(&(fsm->rules), &it)) {
		//Get all actions reachable through other rules
		_fsm_get_actions(&all_actions, nt->start);
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

	//Delete all actions
	Action *st;
	radix_tree_iterator_init(&all_actions, &it);
	while(st = (Action *)radix_tree_iterator_next(&all_actions, &it)) {
		radix_tree_dispose(&st->next);
		c_delete(st);
	}
	radix_tree_iterator_dispose(&all_actions, &it);

	radix_tree_dispose(&all_actions);
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
		_action_init(action, ACTION_TYPE_SHIFT, NONE);
		non_terminal->start = action;
		non_terminal->end = action;
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


Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length)
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
	cursor->current = (Action *)cursor->stack->data;
	cursor->previous = NULL;
	cursor->stack = stack_pop(cursor->stack);
}

void fsm_cursor_reset(FsmCursor *cursor) 
{
	cursor->current = (Action *)cursor->stack->data;
	cursor->previous = NULL;
}

void fsm_cursor_push_continuation(FsmCursor *cursor)
{
	cursor->continuations = stack_push(cursor->continuations, cursor->current);
}

void fsm_cursor_push_new_continuation(FsmCursor *cursor)
{
	Action *action = c_new(Action, 1);
	_action_init(action, ACTION_TYPE_SHIFT, NONE);
	cursor->continuations = stack_push(cursor->continuations, action);
	trace("add", action, NULL, 0, "continuation");
}

Action *fsm_cursor_pop_continuation(FsmCursor *cursor)
{
	Action *action = (Action *)cursor->continuations->data;
	cursor->continuations = stack_pop(cursor->continuations);
	return action;
}

void fsm_cursor_join_continuation(FsmCursor *cursor)
{
	Action *cont = (Action *)cursor->continuations->data;
	int symbol = cursor->last_symbol;

	trace("add", cursor->previous, cont, symbol, "join");

	Action *old = radix_tree_get_int(&cursor->previous->next, symbol);
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
	cur->current = fsm_get_action(cur->fsm, name, length);
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
	cref->action = cur->current;
	cref->non_terminal = nt;
	cref->next = cur->last_non_terminal->child_refs;

	cur->last_non_terminal->child_refs = cref;

	//Parent reference on child
	Reference *pref = c_new(Reference, 1);
	pref->action = cur->current;
	pref->non_terminal = cur->last_non_terminal;
	pref->next = nt->parent_refs;

	nt->parent_refs = pref;
}

Action *_add_action_buffer(Action *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	if(action == NULL) {
		action = c_new(Action, 1);
		_action_init(action, type, reduction);
	}

	//TODO: risk of leak when transitioning using the same symbol
	// on the same action twice.
	radix_tree_set(&from->next, buffer, size, action);
	return action;
}

Action *_add_action(Action *from, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	_action_init(action, type, reduction);
	//TODO: risk of leak when transitioning using the same symbol
	// on the same action twice.
	radix_tree_set_int(&from->next, symbol, action);

	trace("add", from, action, symbol, "action");
	return action;
}

void _add_followset(Action *from, Action *action)
{
	Action *s;
	Iterator it;
	radix_tree_iterator_init(&(action->next), &it);
	while(s = (Action *)radix_tree_iterator_next(&(action->next), &it)) {
		_add_action_buffer(from, it.key, it.size, 0, 0, s);
		trace("add", from, s, buffer_to_symbol(it.key, it.size), "follow");
	}
	radix_tree_iterator_dispose(&(action->next), &it);
}

void _reduce_followset(Action *from, Action *to, int symbol)
{
	Action *s;
	Iterator it;
	radix_tree_iterator_init(&(to->next), &it);
	while(s = (Action *)radix_tree_iterator_next(&(to->next), &it)) {
		_add_action_buffer(from, it.key, it.size, ACTION_TYPE_REDUCE, symbol, NULL);
		trace("add", from, s, buffer_to_symbol(it.key, it.size), "reduce-follow");
	}
	radix_tree_iterator_dispose(&(to->next), &it);
}

Action *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length, int symbol)
{
	cur->current = fsm_get_action(cur->fsm, name, length);
	cur->fsm->start = cur->current;
	//TODO: calling fsm_cursor_set_start multiple times may cause
	// leaks if adding a duplicate accept action to the action.
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
			_add_followset(ref->action, ref->non_terminal->start);
			ref = ref->next;
		}

		ref = nt->parent_refs;
		while(ref) {
			Action *cont = radix_tree_get_int(&ref->action->next, nt->symbol);
			_reduce_followset(nt->end, cont, nt->symbol);
			ref = ref->next;
		}
	}
	radix_tree_iterator_dispose(rules, &it);
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
	cur->previous = cur->current;
	cur->last_symbol = symbol;
	cur->current = action;
}

/**
 * TODO: Deprecated by dynamic action shift
 */
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol)
{
	Action *action = _add_action(cur->current, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE);
	cur->last_non_terminal->end = action;
	cur->previous = cur->current;
	cur->last_symbol = symbol;
	cur->current = action;
}

void fsm_cursor_add_followset(FsmCursor *cur, Action *action)
{
	_add_followset(cur->current, action);
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

	action = radix_tree_get_int(&session->current->next, symbol);
	if(action == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("test", session->current, action, symbol, "error");
			session->current = &session->fsm->error;
		}
		return session->current;
	}

	switch(action->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("test", session->current, action, symbol, "context shift");
		break;
	case ACTION_TYPE_ACCEPT:
		trace("test", session->current, action, symbol, "accept");
		break;
	case ACTION_TYPE_SHIFT:
		trace("test", session->current, action, symbol, "shift");
		break;
	case ACTION_TYPE_REDUCE:
		trace("test", session->current, action, symbol, "reduce");
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
	action = radix_tree_get_int(&session->current->next, symbol);
	if(action == NULL) {
		if(session->current->type != ACTION_TYPE_ACCEPT) {
			trace("match", session->current, action, symbol, "error");
			session->current = &session->fsm->error;
		}
		return;
	}

	switch(action->type) {
	case ACTION_TYPE_CONTEXT_SHIFT:
		trace("match", session->current, action, symbol, "context shift");
		if(session->handler.context_shift) {
			session->handler.context_shift(session->target, session->index, session->length, symbol);
		}
		session_push(session);
		session->current = action;
		break;
	case ACTION_TYPE_ACCEPT:
		trace("match", session->current, action, symbol, "accept");
		session->current = action;
		break;
	case ACTION_TYPE_SHIFT:
		trace("match", session->current, action, symbol, "shift");
		session->current = action;
		break;
	case ACTION_TYPE_REDUCE:
		trace("match", session->current, action, symbol, "reduce");
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
