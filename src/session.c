#include "session.h"

#include "cmemory.h"
#include <stdio.h>

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
#else
#define trace(M, T1, T2, S, A, R)
#endif

void session_init(Session *session, Fsm *fsm)
{
	session->fsm = fsm;
	session->status = SESSION_OK;
	session->current = fsm->start;
	session->last_action = NULL;
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

Session *session_set_handler(Session *session, FsmHandler handler, void *target)
{
	session->handler = handler;
	session->target = target;
}

Action *session_test(Session *session, int symbol, unsigned int index, unsigned int length)
{
	Action *action;
	Action *prev;

	action = radix_tree_get_int(&session->current->actions, symbol);
	if(action == NULL) {
		if(session->last_action->type != ACTION_ACCEPT) {
			trace("test", session->current, action, symbol, "error", 0);
			session->last_action = &session->fsm->error;
			session->current = session->last_action->state;
		}
		return session->last_action;
	}

	switch(action->type) {
	case ACTION_CONTEXT_SHIFT:
		trace("test", session->current, action, symbol, "context shift", 0);
		break;
	case ACTION_ACCEPT:
		trace("test", session->current, action, symbol, "accept", 0);
		break;
	case ACTION_SHIFT:
		trace("test", session->current, action, symbol, "shift", 0);
		break;
	case ACTION_REDUCE:
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
	action = radix_tree_get_int(&session->current->actions, symbol);

	if(action == NULL) {
		// Attempt empty transition
		Symbol *empty = symbol_table_get(session->fsm->table, "__empty", 7);
		action = radix_tree_get_int(&session->current->actions, empty->id);

		if(action == NULL) {
			// Check if accepting state
			if(!session->last_action || session->last_action->type != ACTION_ACCEPT) {
				trace("match", session->current, action, symbol, "error", 0);
				session->last_action = &session->fsm->error;
				session->current = session->last_action->state;
				session->status = SESSION_ERROR;
			}
			return;
		} else {
			trace("match", session->current, action, symbol, "fback", 0);
		}
	}
	session->last_action = action;

	switch(action->type) {
	case ACTION_CONTEXT_SHIFT:
		trace("match", session->current, action, symbol, "context shift", 0);
		if(session->handler.context_shift) {
			session->handler.context_shift(session->target, session->index, session->length, symbol);
		}
		session_push(session);
		session->current = action->state;
		break;
	case ACTION_ACCEPT:
		trace("match", session->current, action, symbol, "accept", 0);
		session->current = action->state;
		break;
	case ACTION_SHIFT:
		trace("match", session->current, action, symbol, "shift", 0);
		session->current = action->state;
		break;
	case ACTION_REDUCE:
		trace("match", session->current, action, symbol, "reduce", action->reduction);
		session_pop(session);
		session->length = index - session->index;
		if(session->handler.reduce) {
			session->handler.reduce(session->target, session->index, session->length, action->reduction);
		}
		session_match(session, action->reduction, session->index, session->length);
		goto rematch; // same as session_match(session, symbol);
		break;
	case ACTION_EMPTY:
		trace("match", session->current, action, symbol, "empty", 0);
		session->current = action->state;
		goto rematch; // same as session_match(session, symbol);
		break;
	default:
		break;
	}
}
