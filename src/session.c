#include "session.h"

#include "cmemory.h"
#include <stdio.h>

#ifdef FSM_TRACE
#define trace(M, ST, T, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		ST, \
		T, \
		T? ((Action*)T)->state: NULL, \
		A, \
		(R != 0)? '>': ' ', \
		R, \
		(S)->symbol, (char)(S)->symbol \
	)
#else
#define trace(M, ST, T, S, A, R)
#endif

void session_init(Session *session, Fsm *fsm, FsmHandler handler)
{
	session->fsm = fsm;
	session->status = SESSION_OK;
	session->current = fsm->start;
	session->last_action = NULL;
	session->stack.top = NULL;
	session->index = 0;
	session->handler = handler;
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

Action *session_test(Session *session, const Token *token)
{
	Action *action;

	action = state_get_transition(session->current, token->symbol);
	if(action == NULL) {
		trace("test", session->current, action, token, "error", 0);
		session->last_action = &session->fsm->error;
		session->current = session->last_action->state;
		return session->last_action;
	}

	switch(action->type) {
	case ACTION_SHIFT:
		trace("test", session->current, action, token, "shift", 0);
		break;
	case ACTION_ACCEPT:
		trace("test", session->current, action, token, "accept", 0);
		break;
	case ACTION_DROP:
		trace("test", session->current, action, token, "drop", 0);
		break;
	case ACTION_REDUCE:
		trace("test", session->current, action, token, "reduce", action->reduction);
		break;
	default:
		break;
	}
	return action;
}

void session_match(Session *session, const Token *token)
{
	Action *action;

rematch:
	session->index = token->index;
	action = state_get_transition(session->current, token->symbol);

	if(action == NULL) {
		// Attempt empty transition
		Symbol *empty = symbol_table_get(session->fsm->table, "__empty", 7);
		action = state_get_transition(session->current, empty->id);

		if(action == NULL) {
			trace("match", session->current, action, token, "error", 0);
			session->last_action = &session->fsm->error;
			session->current = session->last_action->state;
			session->status = SESSION_ERROR;
			return;
		} else {
			trace("match", session->current, action, token, "fback", 0);
		}
	}
	session->last_action = action;

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", session->current, action, token, "shift", 0);
		Token shifted = {
			token->index,
			token->length,
			token->symbol
		};
		if(session->handler.shift) {
			session->handler.shift(session->handler.target, &shifted);
		}
		session_push(session);
		session->current = action->state;
		break;
	case ACTION_ACCEPT:
		trace("match", session->current, action, token, "accept", 0);
		Token accepted = {
			token->index,
			token->length,
			token->symbol
		};
		if(session->handler.accept) {
			session->handler.accept(session->handler.target, &accepted);
		}
		//Restart after accept
		session->current = session->fsm->start;
		break;
	case ACTION_DROP:
		trace("match", session->current, action, token, "drop", 0);
		session->current = action->state;
		break;
	case ACTION_REDUCE:
		trace("match", session->current, action, token, "reduce", action->reduction);
		session_pop(session);
		Token reduction = {
			session->index,
			token->index - session->index,
			action->reduction
		};
		if(session->handler.reduce) {
			session->handler.reduce(session->handler.target, &reduction);
		}
		session_match(session, &reduction);
		goto rematch; // same as session_match(session, symbol);
		break;
	case ACTION_EMPTY:
		trace("match", session->current, action, token, "empty", 0);
		session->current = action->state;
		goto rematch; // same as session_match(session, symbol);
		break;
	default:
		break;
	}
}
