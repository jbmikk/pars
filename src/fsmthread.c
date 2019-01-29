#include "fsmthread.h"

#include "cmemory.h"
#include <stdlib.h>
#include <stdio.h>

#include "dbg.h"

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


FUNCTIONS(Stack, FsmThreadNode, FsmThreadNode, fsmthreadnode);

static void _state_push(FsmThread *thread, FsmThreadNode tnode)
{
	stack_fsmthreadnode_push(&thread->stack, tnode);
}

static FsmThreadNode _state_pop(FsmThread *thread, char type, bool *is_empty)
{
	FsmThreadNode top;

	// TODO: replace is_empty with Maybe(FsmThreadNode) generic.
	while (!(*is_empty = stack_fsmthreadnode_is_empty(&thread->stack))) {
		top = stack_fsmthreadnode_top(&thread->stack);
		stack_fsmthreadnode_pop(&thread->stack);
		if(top.type == type) {
			break;
		}
	}
	return top;
}

static void _mode_push(FsmThread *thread, int symbol)
{
	_state_push(thread, (FsmThreadNode){ 
		FSM_THREAD_NODE_MODE,
		thread->start,
		0
	});
	thread->start = fsm_get_state_by_id(thread->fsm, symbol);
}

static void _mode_pop(FsmThread *thread)
{
	bool is_empty;
	FsmThreadNode popped = _state_pop(thread, FSM_THREAD_NODE_MODE, &is_empty);
	if(is_empty) {
		//sentinel("Mode pop fail");
	}
	thread->start = popped.state;
}

void fsm_thread_init(FsmThread *thread, Fsm *fsm, Listener pipe)
{
	thread->fsm = fsm;
	thread->pipe = pipe;
	thread->start = NULL;
	thread->path = 0;
	stack_fsmthreadnode_init(&thread->stack);
}

void fsm_thread_dispose(FsmThread *thread)
{
	stack_fsmthreadnode_dispose(&thread->stack);
}

bool fsm_thread_stack_is_empty(FsmThread *thread)
{
	bool is_empty = stack_fsmthreadnode_is_empty(&thread->stack);
	bool is_initial = false;
	if(!is_empty) {
		is_initial = thread->stack.stack.top->next == NULL;
	}
	return is_empty || is_initial;
}

int fsm_thread_start(FsmThread *thread)
{
	int symbol = fsm_get_symbol_id(thread->fsm, nzs(".default"));
	thread->start = fsm_get_state_by_id(thread->fsm, symbol);
	thread->transition.from = NULL;
	thread->transition.to = thread->start;
	// TODO: Is this initial node really needed for EBNF?
	// If removed, rewrite fsm_thread_stack_is_empty
	_state_push(thread, (FsmThreadNode){ 
		FSM_THREAD_NODE_SR,
		thread->transition.to,
		0
	});
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
	bool is_empty;
	switch(action->type) {
	case ACTION_REDUCE:
		popped = _state_pop(thread, FSM_THREAD_NODE_SR, &is_empty);
		if(is_empty) {
			//sentinel("Reduce pop fail");
		}
		t.to = popped.state;

		Token reduction = {
			popped.index,
			t.token.index - popped.index,
			action->reduction
		};
		// TODO: Shouldn't the "input stack" be handled by the input
		// loop? If we do this we could completely remove the pda 
		// loop and just return a continuation indicating something
		// has been pushed. This way we would have just pipes and no
		// loops, except for the main input loop.
		// This might work for the input loop, but not for the lexer
		// to parser pipe. We really need a loop there.
		// Maybe what we really need to is convert the lexer output
		// into a proper input. The pda loop right now works as some
		// sort of base input loop upon which we can build other more
		// specialized loops, maybe we should reify that.
		t.reduction = reduction;
		break;
	case ACTION_SHIFT:
		_state_push(thread, (FsmThreadNode) {
			FSM_THREAD_NODE_SR,
			t.from,
			t.token.index
		});
	}
	return t;
}

static Transition _start_accept(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	FsmThreadNode popped;
	bool is_empty;
	switch(action->type) {
	case ACTION_START:
		_state_push(thread, (FsmThreadNode) {
			FSM_THREAD_NODE_SA,
			t.from,
			t.token.index
		});
		break;
	case ACTION_ACCEPT:
		t.to = thread->start;
		popped = _state_pop(thread, FSM_THREAD_NODE_SA, &is_empty);
		if(is_empty) {
			//sentinel("Accept pop fail");
		}

		Token reduction = {
			popped.index,
			t.token.index - popped.index,
			action->reduction
		};
		t.reduction = reduction;
	}
	return t;
}

static Transition _backtrack(Transition transition, FsmThread *thread) {
	Transition t = transition;
	char alt_path = transition.path + 1;
	Action *alt_action = state_get_path_transition(transition.from, transition.token.symbol, alt_path);

	if(alt_action) {
		_state_push(thread, (FsmThreadNode) {
			FSM_THREAD_NODE_BACKTRACK,
			transition.from,
			transition.token.index,
			transition.path
		});
	} else if (transition.action->type == ACTION_ERROR) {
		t.backtrack = 0;
		bool is_empty;
		FsmThreadNode popped = _state_pop(thread, FSM_THREAD_NODE_BACKTRACK, &is_empty);
		if(!is_empty) {
			// TODO: Review the backtrack and path variables.
			// The only problem here: it is not true that we are
			// transitioning to this state with this symbol. This
			// symbol transitioned to error, now we are reseting 
			// the world. Maybe we should reify that.
			// TODO: Communicate we have backtracked!!!!
			t.to = popped.state;
			t.token.index = popped.index;
			t.backtrack = 1;
			thread->path = popped.path + 1;
		}
	}
	return t;
}

Transition fsm_thread_match(FsmThread *thread, const Token *token)
{
	Action *action;
	Transition prev = thread->transition;
	Transition next;
	char path = thread->path;

	action = state_get_path_transition(prev.to, token->symbol, path);

	if(action == NULL) {
		// Attempt empty transition
		int empty = fsm_get_symbol_id(thread->fsm, nzs("__empty"));
		// TODO: Validate backtracking with empty transitions
		action = state_get_path_transition(prev.to, empty, path);

		if(action == NULL) {
			State *error = fsm_get_state(thread->fsm, nzs(".error"));
			action = state_get_transition(error, empty);
		}
	}

	next.action = action;
	next.from = prev.to;
	next.to = action->state;
	next.token = *token;
	next.path = path;

	// Reset path
	thread->path = 0;
	return next;
}

void fsm_thread_apply(FsmThread *thread, Transition transition)
{
	// TODO: Consider converting backtracking stack into a loop pipe.
	// Other features, such as shift/reduce stack could also be turned 
	// into pipes. This implies pipes can have state, maybe stored in the 
	// context structure?
	// We should consider which things change and which don't at each step
	Transition t = transition;
	t = _backtrack(t, thread);
	t = _shift_reduce(t, thread);
	t = _start_accept(t, thread);
	t = _switch_mode(t, thread);

	thread->transition = t;
}

static Continuation _build_continuation(Transition t, int error)
{
	Continuation cont;
	token_init(&cont.token2, 0, 0, 0);
	cont.token = t.token;

	if(error) {
		cont.type = CONTINUATION_ERROR; 
	} else {
		switch(t.action->type) {
		case ACTION_START:
		case ACTION_ACCEPT:
		case ACTION_SHIFT:
		case ACTION_DROP:
			cont.type = CONTINUATION_NEXT; 
			break;
		case ACTION_EMPTY:
			cont.type = CONTINUATION_RETRY; 
			break;
		case ACTION_REDUCE:
			cont.type = CONTINUATION_PUSH; 
			cont.token2 = t.reduction;
			break;
		case ACTION_ERROR:
			if(t.backtrack > 0) {
				cont.type = CONTINUATION_RETRY;
			} else {
				cont.type = CONTINUATION_ERROR; 
			}
			break;
		default:
			// sentinel?
			break;
		}
	}

	return cont;
}

Continuation fsm_thread_cycle(FsmThread *thread, const Token token)
{
	Transition transition;

	transition = fsm_thread_match(thread, &token);
	_trace_transition(transition, thread->transition);

	fsm_thread_apply(thread, transition);
	transition = thread->transition;

	int error = listener_notify(&thread->pipe, &transition);

	// Listener's return value is combined with the transition to
	// get the continuation.
	return _build_continuation(transition, error);
}
