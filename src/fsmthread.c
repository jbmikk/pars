#include "fsmthread.h"

#include "cmemory.h"
#include <stdlib.h>
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

static void _mode_push(FsmThread *thread, int symbol)
{
	ModeNode *node = malloc(sizeof(ModeNode));
	node->state = fsm_get_state_by_id(thread->fsm, symbol);
	node->next = thread->mode_stack.top;
	thread->mode_stack.top = node;
}

static void _mode_pop(FsmThread *thread)
{
	ModeNode *top = thread->mode_stack.top;
	thread->mode_stack.top = top->next;
	free(top);
}

static void _mode_reset(FsmThread *thread)
{
	thread->current = thread->mode_stack.top->state;
}

static void _state_push(FsmThread *thread, unsigned int index)
{
	FsmThreadNode *node = malloc(sizeof(FsmThreadNode));
	node->state = thread->current;
	node->index = index;
	node->next = thread->stack.top;
	thread->stack.top = node;
}

static unsigned int _state_pop(FsmThread *thread)
{
	FsmThreadNode *top = thread->stack.top;
	unsigned int index = top->index;
	thread->current = top->state;
	thread->stack.top = top->next;
	free(top);
	return index;
}


void fsm_thread_init(FsmThread *thread, Fsm *fsm)
{
	thread->fsm = fsm;
	thread->status = FSM_THREAD_OK;
	thread->stack.top = NULL;
	thread->mode_stack.top = NULL;
	thread->handler = NULL_HANDLER;
}

void fsm_thread_dispose(FsmThread *thread)
{
	while(thread->stack.top) {
		_state_pop(thread);
	}
	while(thread->mode_stack.top) {
		_mode_pop(thread);
	}
}

int fsm_thread_start(FsmThread *thread)
{
	_mode_push(thread, fsm_get_symbol_id(thread->fsm, nzs(".default")));
	_mode_reset(thread);
	_state_push(thread, 0);
	//TODO: Check errors?
	return 0;
}


Action *fsm_thread_test(FsmThread *thread, const Token *token)
{
	Action *action;

	action = state_get_transition(thread->current, token->symbol);
	if(action == NULL) {
		Symbol *empty = symbol_table_get(thread->fsm->table, "__empty", 7);

		trace("test", thread->current, action, token, "error", 0);
		_mode_push(thread, fsm_get_symbol_id(thread->fsm, nzs(".error")));
		_mode_reset(thread);
		action = state_get_transition(thread->current, empty->id);
	}

	switch(action->type) {
	case ACTION_SHIFT:
		trace("test", thread->current, action, token, "shift", 0);
		break;
	case ACTION_ACCEPT:
		trace("test", thread->current, action, token, "accept", 0);
		break;
	case ACTION_DROP:
		trace("test", thread->current, action, token, "drop", 0);
		break;
	case ACTION_REDUCE:
		trace("test", thread->current, action, token, "reduce", action->reduction);
		break;
	default:
		break;
	}
	return action;
}

Action *fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;

	action = state_get_transition(thread->current, token->symbol);

	if(action == NULL) {
		// Attempt empty transition
		Symbol *empty = symbol_table_get(thread->fsm->table, "__empty", 7);
		action = state_get_transition(thread->current, empty->id);

		if(action == NULL) {
			trace("match", thread->current, action, token, "error", 0);
			_mode_push(thread, fsm_get_symbol_id(thread->fsm, nzs(".error")));
			_mode_reset(thread);
			action = state_get_transition(thread->current, empty->id);
		} else {
			trace("match", thread->current, action, token, "fback", 0);
		}
	}

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", thread->current, action, token, "shift", 0);
		Token shifted = {
			token->index,
			token->length,
			token->symbol
		};
		if(thread->handler.shift) {
			thread->handler.shift(thread->handler.target, &shifted);
		}
		_state_push(thread, token->index);
		thread->current = action->state;
		break;
	case ACTION_ACCEPT:
		trace("match", thread->current, action, token, "accept", 0);
		Token accepted = {
			token->index,
			token->length,
			token->symbol
		};
		if(thread->handler.accept) {
			thread->handler.accept(thread->handler.target, &accepted);
		}

		if(action->flags & ACTION_FLAG_MODE_PUSH) {
			_mode_push(thread, action->mode);
		} else if(action->flags & ACTION_FLAG_MODE_POP) {
			_mode_pop(thread);
		}
		_mode_reset(thread);
		break;
	case ACTION_DROP:
		trace("match", thread->current, action, token, "drop", 0);
		Token dropped = {
			token->index,
			token->length,
			token->symbol
		};
		if(thread->handler.drop) {
			thread->handler.drop(thread->handler.target, &dropped);
		}
		thread->current = action->state;
		//if(action->flags & ACTION_FLAG_THREAD_SPAWN)
			//_thread_spawn(thread);
		break;
	case ACTION_REDUCE:
		trace("match", thread->current, action, token, "reduce", action->reduction);
		unsigned int popped_index = _state_pop(thread);
		Token reduction = {
			popped_index,
			token->index - popped_index,
			action->reduction
		};
		if(thread->handler.reduce) {
			thread->handler.reduce(thread->handler.target, &reduction);
		}
		fsm_thread_match(thread, &reduction);
		action = fsm_thread_match(thread, token);
		break;
	case ACTION_EMPTY:
		trace("match", thread->current, action, token, "empty", 0);
		thread->current = action->state;
		action = fsm_thread_match(thread, token);
		break;
	case ACTION_ERROR:
		thread->status = FSM_THREAD_ERROR;
	default:
		break;
	}
	return action;
}
