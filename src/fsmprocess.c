#include "fsmprocess.h"

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

static void _mode_push(FsmProcess *process, int symbol)
{
	ModeNode *node = c_new(ModeNode, 1);
	node->state = fsm_get_state_by_id(process->fsm, symbol);
	node->next = process->mode_stack.top;
	process->mode_stack.top = node;
}

static void _mode_pop(FsmProcess *process)
{
	ModeNode *top = process->mode_stack.top;
	process->mode_stack.top = top->next;
	c_delete(top);
}

static void _mode_reset(FsmProcess *process)
{
	process->current = process->mode_stack.top->state;
}

static void _state_push(FsmProcess *process, unsigned int index)
{
	FsmProcessNode *node = c_new(FsmProcessNode, 1);
	node->state = process->current;
	node->index = index;
	node->next = process->stack.top;
	process->stack.top = node;
}

static unsigned int _state_pop(FsmProcess *process)
{
	FsmProcessNode *top = process->stack.top;
	unsigned int index = top->index;
	process->current = top->state;
	process->stack.top = top->next;
	c_delete(top);
	return index;
}


void fsm_process_init(FsmProcess *process, Fsm *fsm)
{
	process->fsm = fsm;
	process->status = FSM_THREAD_OK;
	process->stack.top = NULL;
	process->mode_stack.top = NULL;
	process->handler = NULL_HANDLER;
}

void fsm_process_dispose(FsmProcess *process)
{
	while(process->stack.top) {
		_state_pop(process);
	}
	while(process->mode_stack.top) {
		_mode_pop(process);
	}
}

int fsm_process_start(FsmProcess *process)
{
	_mode_push(process, fsm_get_symbol_id(process->fsm, nzs(".default")));
	_mode_reset(process);
	_state_push(process, 0);
	//TODO: Check errors?
	return 0;
}


Action *fsm_process_test(FsmProcess *process, const Token *token)
{
	Action *action;

	action = state_get_transition(process->current, token->symbol);
	if(action == NULL) {
		Symbol *empty = symbol_table_get(process->fsm->table, "__empty", 7);

		trace("test", process->current, action, token, "error", 0);
		_mode_push(process, fsm_get_symbol_id(process->fsm, nzs(".error")));
		_mode_reset(process);
		action = state_get_transition(process->current, empty->id);
	}

	switch(action->type) {
	case ACTION_SHIFT:
		trace("test", process->current, action, token, "shift", 0);
		break;
	case ACTION_ACCEPT:
		trace("test", process->current, action, token, "accept", 0);
		break;
	case ACTION_DROP:
		trace("test", process->current, action, token, "drop", 0);
		break;
	case ACTION_REDUCE:
		trace("test", process->current, action, token, "reduce", action->reduction);
		break;
	default:
		break;
	}
	return action;
}

Action *fsm_process_match(FsmProcess *process, const Token *token)
{
	Action *action;

	action = state_get_transition(process->current, token->symbol);

	if(action == NULL) {
		// Attempt empty transition
		Symbol *empty = symbol_table_get(process->fsm->table, "__empty", 7);
		action = state_get_transition(process->current, empty->id);

		if(action == NULL) {
			trace("match", process->current, action, token, "error", 0);
			_mode_push(process, fsm_get_symbol_id(process->fsm, nzs(".error")));
			_mode_reset(process);
			action = state_get_transition(process->current, empty->id);
		} else {
			trace("match", process->current, action, token, "fback", 0);
		}
	}

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", process->current, action, token, "shift", 0);
		Token shifted = {
			token->index,
			token->length,
			token->symbol
		};
		if(process->handler.shift) {
			process->handler.shift(process->handler.target, &shifted);
		}
		_state_push(process, token->index);
		process->current = action->state;
		break;
	case ACTION_ACCEPT:
		trace("match", process->current, action, token, "accept", 0);
		Token accepted = {
			token->index,
			token->length,
			token->symbol
		};
		if(process->handler.accept) {
			process->handler.accept(process->handler.target, &accepted);
		}

		if(action->flags & ACTION_FLAG_MODE_PUSH) {
			_mode_push(process, action->mode);
		} else if(action->flags & ACTION_FLAG_MODE_POP) {
			_mode_pop(process);
		}
		_mode_reset(process);
		break;
	case ACTION_DROP:
		trace("match", process->current, action, token, "drop", 0);
		Token dropped = {
			token->index,
			token->length,
			token->symbol
		};
		if(process->handler.drop) {
			process->handler.drop(process->handler.target, &dropped);
		}
		process->current = action->state;
		break;
	case ACTION_REDUCE:
		trace("match", process->current, action, token, "reduce", action->reduction);
		unsigned int popped_index = _state_pop(process);
		Token reduction = {
			popped_index,
			token->index - popped_index,
			action->reduction
		};
		if(process->handler.reduce) {
			process->handler.reduce(process->handler.target, &reduction);
		}
		fsm_process_match(process, &reduction);
		action = fsm_process_match(process, token);
		break;
	case ACTION_EMPTY:
		trace("match", process->current, action, token, "empty", 0);
		process->current = action->state;
		action = fsm_process_match(process, token);
		break;
	case ACTION_ERROR:
		process->status = FSM_THREAD_ERROR;
	default:
		break;
	}
	return action;
}
