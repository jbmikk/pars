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

DEFINE_STACK_FUNCTIONS(State *, State, state, IMPLEMENTATION);

DEFINE_STACK_FUNCTIONS(FsmThreadNode, FsmThreadNode, fsmthreadnode, IMPLEMENTATION);

static void _mode_push(FsmThread *thread, int symbol)
{
	State *state = fsm_get_state_by_id(thread->fsm, symbol);
	stack_state_push(&thread->mode_stack, state);
}

static void _mode_pop(FsmThread *thread)
{
	stack_state_pop(&thread->mode_stack);
}

static void _mode_reset(FsmThread *thread)
{
	thread->current = stack_state_top(&thread->mode_stack);
}

static void _state_push(FsmThread *thread, FsmThreadNode tnode)
{
	stack_fsmthreadnode_push(&thread->stack, tnode);
}

static FsmThreadNode _state_pop(FsmThread *thread)
{
	FsmThreadNode top = stack_fsmthreadnode_top(&thread->stack);
	stack_fsmthreadnode_pop(&thread->stack);
	return top;
}


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Output *output)
{
	thread->fsm = fsm;
	stack_fsmthreadnode_init(&thread->stack);
	stack_state_init(&thread->mode_stack);
	thread->handler = NULL_HANDLER;
	thread->output = output;
}

void fsm_thread_dispose(FsmThread *thread)
{
	stack_fsmthreadnode_dispose(&thread->stack);
	stack_state_dispose(&thread->mode_stack);
}

int fsm_thread_start(FsmThread *thread)
{
	_mode_push(thread, fsm_get_symbol_id(thread->fsm, nzs(".default")));
	_mode_reset(thread);
	_state_push(thread, (FsmThreadNode){ thread->current, 0 });
	//TODO: Check errors?
	return 0;
}

Continuation fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;
	Continuation cont;

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

	cont.action = action;

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", thread->current, action, token, "shift", 0);
		Token shifted = *token;
		if(thread->handler.shift) {
			thread->handler.shift(thread->handler.target, &shifted);
		}
		_state_push(thread, (FsmThreadNode) {
			thread->current,
			token->index
		});
		thread->current = action->state;
		cont.token = shifted;
		break;
	case ACTION_ACCEPT:
		trace("match", thread->current, action, token, "accept", 0);
		Token accepted = *token;
		if(thread->handler.accept) {
			thread->handler.accept(thread->handler.target, &accepted);
		}

		if(action->flags & ACTION_FLAG_MODE_PUSH) {
			_mode_push(thread, action->mode);
		} else if(action->flags & ACTION_FLAG_MODE_POP) {
			_mode_pop(thread);
		}
		_mode_reset(thread);
		cont.token = accepted;
		break;
	case ACTION_DROP:
		trace("match", thread->current, action, token, "drop", 0);
		Token dropped = *token;
		if(thread->handler.drop) {
			thread->handler.drop(thread->handler.target, &dropped);
		}
		thread->current = action->state;
		cont.token = dropped;
		//if(action->flags & ACTION_FLAG_THREAD_SPAWN)
			//_thread_spawn(thread);
		break;
	case ACTION_REDUCE:
		trace("match", thread->current, action, token, "reduce", action->reduction);
		FsmThreadNode popped = _state_pop(thread);
		thread->current = popped.state;

		Token reduction = {
			popped.index,
			token->index - popped.index,
			action->reduction
		};
		if(thread->handler.reduce) {
			thread->handler.reduce(thread->handler.target, &reduction);
		}
		// TODO: Calling thread_match recursively is efficient, but it
		// seems like we are breaking something. It would be desirable
		// to have the proper status reflected in the thread structure
		// and wait for the next match call with the proper data.
		// Maybe we can have an improved input feature that lets us
		// place things on top of the stack for future input.
		// Something generic can be useful for several scenarios
		// besides reductions, such as back-tracking.
		// Maybe we can have decorated inputs, or decorated tokens.
		// Something indicating the input is different (it comes from
		// back-tracking, or something else.)
		cont.token = reduction;
		break;
	case ACTION_EMPTY:
		trace("match", thread->current, action, token, "empty", 0);
		thread->current = action->state;
		cont.token = *(token);
		break;
	case ACTION_ERROR:
		output_raise(thread->output, OUTPUT_FSM_ERROR);
	default:
		break;
	}
	return cont;
}

// TODO: This code belongs elsewhere
int pda_continuation_follow(const Continuation *cont, const Token *in, Token *out, int *count)
{
	switch(cont->action->type) {
	case ACTION_SHIFT:
	case ACTION_ACCEPT:
	case ACTION_DROP:
		if(*count == 0) {
			// In token was matched, end loop
			return 1;
		} else {
			// In token generated a reduction the last time
			// Try matching it again.
			(*count)--;
			*out = *in;
			return 0;
		}
		break;
	case ACTION_REDUCE:
		(*count)++;
		*out = cont->token;
		break;
	case ACTION_EMPTY:
		*out = cont->token;
		break;
	case ACTION_ERROR:
		return -1;
	default:
		return -2;
		break;
	}
	return 0;
}

