#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"

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
#else
#define trace(M, T1, T2, S, A, R)
#endif

#define NONE 0

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

void fsm_init(Fsm *fsm, SymbolTable *table)
{
	//TODO: Get symbol table as parameter
	fsm->table = table;
	fsm->start = NULL;
	_action_init(&fsm->error, ACTION_TYPE_ERROR, NONE, NULL);
	fsm->error.state = c_new(State, 1);
	_state_init(fsm->error.state);
}

void _fsm_get_actions(Node *actions, Node *states, Action *action)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	//TODO: Should have a separate ptr_to_array function
	int_to_array(buffer, &size, (intptr_t)action);

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
			int_to_array(buffer, &size, (intptr_t)state);
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
	Symbol *symbol;
	NonTerminal *nt;
	Iterator it;

	radix_tree_init(&all_actions, 0, 0, NULL);
	radix_tree_init(&all_states, 0, 0, NULL);

	//Get all actions reachable through the starting action
	if(fsm->start) {
		_fsm_get_actions(&all_actions, &all_states, fsm->start);
	}

	radix_tree_iterator_init(&it, &fsm->table->symbols);
	while(symbol = (Symbol *)radix_tree_iterator_next(&it)) {
		//Get all actions reachable through other rules
		nt = (NonTerminal *)symbol->data;

		if(!nt) {
			//Some symbols may not have non terminals
			//TODO: Should we have a separate non terminals array?
			continue;
		}

		_fsm_get_actions(&all_actions, &all_states, nt->start);
		Reference *ref = nt->parent_refs;
		while(ref) {
			Reference *pref = ref;
			ref = ref->next;
			c_delete(pref);
		}
		c_delete(nt);
		//TODO: Symbol table may live longer than fsm, makes sense?
		symbol->data = NULL;
	}
	radix_tree_iterator_dispose(&it);

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

	fsm->table = NULL;
}

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? (NonTerminal *)symbol->data: NULL;
}

Symbol *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_add(fsm->table, name, length);
	NonTerminal *non_terminal;
	Action *action;
	if(!symbol->data) {
		non_terminal = c_new(NonTerminal, 1);
		action = c_new(Action, 1);
		_action_init(action, ACTION_TYPE_SHIFT, NONE, NULL);
		non_terminal->start = action;
		non_terminal->end = action;
		non_terminal->parent_refs = NULL;
		non_terminal->unsolved_returns = 0;
		non_terminal->unsolved_invokes = 0;
		symbol->data = non_terminal;
		//TODO: Add to non_terminal struct: 
		// * detect circular references.
	}
	return symbol;
}


Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start;
}

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start->state;
}

int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? symbol->id: 0;
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
