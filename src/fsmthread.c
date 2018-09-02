#include "fsmthread.h"

#include "cmemory.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef FSM_TRACE
#define trace(M, ST, T, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %i %c %3i (%3i=%2c)\n", \
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

FUNCTIONS(Stack, State *, State, state);

FUNCTIONS(Stack, FsmThreadNode, FsmThreadNode, fsmthreadnode);

static void _mode_push(FsmThread *thread, int symbol)
{
	stack_state_push(&thread->mode_stack, thread->start);
	thread->start = fsm_get_state_by_id(thread->fsm, symbol);
}

static void _mode_pop(FsmThread *thread)
{
	thread->start = stack_state_top(&thread->mode_stack);
	stack_state_pop(&thread->mode_stack);
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


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Listener pipe)
{
	thread->fsm = fsm;
	thread->pipe = pipe;
	thread->start = NULL;
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
	int symbol = fsm_get_symbol_id(thread->fsm, nzs(".default"));
	thread->start = fsm_get_state_by_id(thread->fsm, symbol);
	thread->transition.from = NULL;
	thread->transition.to = thread->start;
	_state_push(thread, (FsmThreadNode){ thread->transition.to, 0 });
	//TODO: Check errors?
	return 0;
}

static void _trace_transition(Transition next, Transition prev) {
	trace("match", 
		prev.state, 
		next.action,
		&next.token, 
		next.action->type, 
		next.action->reduction
	);
}

static Transition _switch_mode(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	if(action->flags & ACTION_FLAG_MODE_PUSH) {
		_mode_push(thread, action->mode);
		t.to = thread->start;
	} else if(action->flags & ACTION_FLAG_MODE_POP) {
		_mode_pop(thread);
		t.to = thread->start;
	}
	return t;
}

static Transition _shift_reduce(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	FsmThreadNode popped;
	switch(action->type) {
	case ACTION_REDUCE:
		popped = _state_pop(thread);
		t.to = popped.state;

		Token reduction = {
			popped.index,
			t.token.index - popped.index,
			action->reduction
		};
		t.token = reduction;
		break;
	case ACTION_SHIFT:
		_state_push(thread, (FsmThreadNode) {
			t.from,
			t.token.index
		});
	}
	return t;
}

static Transition _accept(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	switch(action->type) {
	case ACTION_ACCEPT:
		t.to = thread->start;
	}
	return t;
}

Transition fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;
	Transition prev = thread->transition;
	Transition next;

	action = state_get_transition(prev.to, token->symbol);

	if(action == NULL) {
		// Attempt empty transition
		int empty = fsm_get_symbol_id(thread->fsm, nzs("__empty"));
		action = state_get_transition(prev.to, empty);

		if(action == NULL) {
			State *error = fsm_get_state(thread->fsm, nzs(".error"));
			action = state_get_transition(error, empty);
		}
	}

	next.action = action;
	next.from = prev.to;
	next.to = action->state;
	next.token = *token;

	return next;
}

void fsm_thread_apply(FsmThread *thread, Transition transition)
{
	Transition t = transition;
	t = _shift_reduce(t, thread);
	t = _accept(t, thread);
	t = _switch_mode(t, thread);

	thread->transition = t;
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


Continuation fsm_thread_loop(FsmThread *thread, const Token token)
{
	int count = 0;

	Token retry = token;
	Transition transition;
	Continuation cont;
	do {
		transition = fsm_thread_match(thread, &retry);
		_trace_transition(transition, thread->transition);

		fsm_thread_apply(thread, transition);
		transition = thread->transition;

		int error = listener_notify(&thread->pipe, &transition);

		// Listener's return value is combined with the transition to
		// get the continuation.
		cont.transition = transition;
		cont.error = error || transition.action->type == ACTION_ERROR;

		// TODO: Temporary transition, it should be in control loop
	} while (!_continuation_follow(&cont, &token, &retry, &count));
	return cont;
}
