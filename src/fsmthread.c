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

State *_mode_start(FsmThread *thread)
{
	return stack_state_top(&thread->mode_stack);
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
	thread->transition.state = _mode_start(thread);
	_state_push(thread, (FsmThreadNode){ thread->transition.state, 0 });
	//TODO: Check errors?
	return 0;
}

Transition fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;
	Transition prev = thread->transition;
	Transition next;

	action = state_get_transition(prev.state, token->symbol);

	if(action == NULL) {
		// Attempt empty transition
		Symbol *empty = symbol_table_get(thread->fsm->table, "__empty", 7);
		action = state_get_transition(prev.state, empty->id);

		if(action == NULL) {
			trace("match", prev.state, action, token, "error", 0);
			_mode_push(thread, fsm_get_symbol_id(thread->fsm, nzs(".error")));
			thread->transition.state = _mode_start(thread);
			action = state_get_transition(thread->transition.state, empty->id);
		} else {
			trace("match", prev.state, action, token, "fback", 0);
		}
	}

	next.action = action;

	switch(action->type) {
	case ACTION_SHIFT:
		trace("match", prev.state, action, token, "shift", 0);

		_state_push(thread, (FsmThreadNode) {
			prev.state,
			token->index
		});
		next.state = action->state;
		next.token = *token;
		break;
	case ACTION_ACCEPT:
		trace("match", prev.state, action, token, "accept", 0);

		if(action->flags & ACTION_FLAG_MODE_PUSH) {
			_mode_push(thread, action->mode);
		} else if(action->flags & ACTION_FLAG_MODE_POP) {
			_mode_pop(thread);
		}
		next.state = _mode_start(thread);
		next.token = *token;
		break;
	case ACTION_DROP:
		trace("match", prev.state, action, token, "drop", 0);

		next.state = action->state;
		next.token = *token;
		//if(action->flags & ACTION_FLAG_THREAD_SPAWN)
			//_thread_spawn(thread);
		break;
	case ACTION_REDUCE:
		trace("match", prev.state, action, token, "reduce", action->reduction);
		FsmThreadNode popped = _state_pop(thread);
		next.state = popped.state;

		Token reduction = {
			popped.index,
			token->index - popped.index,
			action->reduction
		};
		next.token = reduction;
		break;
	case ACTION_EMPTY:
		trace("match", prev.state, action, token, "empty", 0);
		next.state = action->state;
		next.token = *token;
		break;
	default:
		// TODO: sentinel?
		break;
	}
	thread->transition = next;
	return next;
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
