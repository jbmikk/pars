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


void fsm_thread_init(FsmThread *thread, Fsm *fsm)
{
	thread->fsm = fsm;
	stack_fsmthreadnode_init(&thread->stack);
	stack_state_init(&thread->mode_stack);
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

Transition fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;
	Transition tran;

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

	tran.action = action;

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", thread->current, action, token, "shift", 0);
		Token shifted = *token;

		_state_push(thread, (FsmThreadNode) {
			thread->current,
			token->index
		});
		thread->current = action->state;
		tran.token = shifted;
		break;
	case ACTION_ACCEPT:
		trace("match", thread->current, action, token, "accept", 0);
		Token accepted = *token;

		if(action->flags & ACTION_FLAG_MODE_PUSH) {
			_mode_push(thread, action->mode);
		} else if(action->flags & ACTION_FLAG_MODE_POP) {
			_mode_pop(thread);
		}
		_mode_reset(thread);
		tran.token = accepted;
		break;
	case ACTION_DROP:
		trace("match", thread->current, action, token, "drop", 0);
		Token dropped = *token;

		thread->current = action->state;
		tran.token = dropped;
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
		tran.token = reduction;
		break;
	case ACTION_EMPTY:
		trace("match", thread->current, action, token, "empty", 0);
		thread->current = action->state;
		tran.token = *(token);
		break;
	default:
		break;
	}
	return tran;
}

// TODO: Does this code belongs elsewhere?
static int _continuation_follow(const Continuation *cont, const Token *in, Token *out, int *count)
{
	// TODO: Add constants for errors / control codes
	int ret;

	switch(cont->transition.action->type) {
	case ACTION_SHIFT:
	case ACTION_ACCEPT:
	case ACTION_DROP:
		if(*count == 0) {
			// In token was matched, end loop
			ret = 1;
		} else {
			// In token generated a reduction the last time
			// Try matching it again.
			(*count)--;
			*out = *in;
			ret = cont->error;
		}
		break;
	case ACTION_REDUCE:
		(*count)++;
		*out = cont->transition.token;
		ret = cont->error;
		break;
	case ACTION_EMPTY:
		*out = cont->transition.token;
		ret = cont->error;
		break;
	case ACTION_ERROR:
		ret = -1;
		break;
	default:
		ret = -2;
		break;
	}
	return ret;
}

Continuation fsm_pda_loop(FsmThread *thread, const Token token, Listener listener)
{
	int count = 0;

	Token retry = token;
	Continuation cont;
	do {
		cont.transition = fsm_thread_match(thread, &retry);
		int error = listener_notify(&listener, &cont.transition);
		cont.error = error || cont.transition.action->type == ACTION_ERROR;

		// TODO: Temporary transition, it should be in control loop
	} while (!_continuation_follow(&cont, &token, &retry, &count));
	return cont;
}
