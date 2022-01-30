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


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Listener pipe)
{
	thread->fsm = fsm;
	thread->pipe = pipe;
	thread->path = 0;
	pdastack_init(&thread->stack);
}

void fsm_thread_dispose(FsmThread *thread)
{
	pdastack_dispose(&thread->stack);
}

bool fsm_thread_stack_is_empty(FsmThread *thread)
{
	return pdastack_is_empty(&thread->stack);
}

int fsm_thread_start(FsmThread *thread)
{
	int symbol = fsm_get_symbol_id(thread->fsm, nzs(".default"));
	State *start = fsm_get_state_by_id(thread->fsm, symbol);
	pdastack_start(&thread->stack, start);
	thread->transition.from = NULL;
	thread->transition.to = start;
	//TODO: Check errors?
	return 0;
}

// Testing only
int fsm_thread_fake_start(FsmThread *thread, State *state)
{
	fsm_thread_start(thread);
	thread->transition.to = state;

	pdastack_state_push(&thread->stack, (PDANode){ 
		.type = PDA_NODE_SR,
		.state = thread->transition.to,
		.token = { 0, 0, 0 }
	});
	//TODO: Check errors?
	return 0;
}

static void _trace_transition(Transition next, Transition prev) {
	trace("match", 
		prev.from, 
		next.action,
		&next.token, 
		next.action->type, 
		next.action->reduction
	);
}

static Transition _switch_mode(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	State *start;
	if(action->flags & ACTION_FLAG_MODE_PUSH) {
		start = fsm_get_state_by_id(thread->fsm, action->mode);
		pdastack_mode_push(&thread->stack, start);
		t.to = start;
	} else if(action->flags & ACTION_FLAG_MODE_POP) {
		start = pdastack_mode_pop(&thread->stack);
		t.to = start;
	}
	return t;
}

static Transition _pop_reductions(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	switch(action->type) {
	case ACTION_POP:
	case ACTION_POP_SHIFT:
		pdastack_pop(&thread->stack);
		break;
	}
	return t;
}

static Transition _shift_reduce(Transition transition, FsmThread *thread) {
	Action *action = transition.action;
	Transition t = transition;
	PDANode popped;
	bool is_empty;
	switch(action->type) {
	case ACTION_REDUCE:
		popped = pdastack_state_pop(&thread->stack, PDA_NODE_SR, &is_empty);
		if(is_empty) {
			//sentinel("Reduce pop fail");
			printf("FTH %p: SR pop fail\n", thread);
			exit(1);
		}
		// TODO: Figure out how to translate this operation into a
		// regular PDA transition. Setting the current state to a 
		// value from the stack does not seem to be possible with a
		// regular PDA, where actions are something like this: 
		//   state1--input(pop-symbol1/push-symbol2)-->state2
		// If we use state2 instead of the stack then the parser is
		// not LALR anymore (or is it?). When reductions are unaware
		// of the state the parser transitions to, they work with any
		// state they pop, which makes LALR more powerful.
		// Maybe we should add a PDA feature to make use of this kind
		// of jumps. Something like a flag or a special symbol so
		// reductions set the current state to the one on top of the 
		// stack. Something like:
		//   state1--input(pop-symbol/push-symbol)-->pop-symbol
		// With this special symbol there's no need to distinguish
		// shifts from reductions, they become particular cases.
		t.to = popped.state;

		Token reduction = {
			popped.token.index,
			t.token.index - popped.token.index,
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
		pdastack_state_push(&thread->stack, (PDANode) {
			.type = PDA_NODE_REDUCTION,
			.state = NULL,
			.token = reduction
		});
		break;
	case ACTION_POP_SHIFT:
		pdastack_state_push(&thread->stack, (PDANode) {
			.type = PDA_NODE_SR,
			.state = t.from,
			.token = { t.popped.index, 0, 0 }
		});
		break;
	case ACTION_SHIFT:
		pdastack_state_push(&thread->stack, (PDANode) {
			.type = PDA_NODE_SR,
			.state = t.from,
			.token = { t.token.index, 0, 0 }
		});
		break;
	}
	return t;
}

static Transition _start_accept(Transition transition, FsmThread *thread, Transition prev) {
	Action *action = transition.action;
	Transition t = transition;
	PDANode popped;
	bool is_empty;
	// Only lexers have SA actions
	switch(action->type) {
	case ACTION_START:
		pdastack_state_push(&thread->stack, (PDANode) {
			.type = PDA_NODE_SA,
			.state = t.from,
			.token = { t.token.index, 0, 0 }
		});
		break;
	case ACTION_ACCEPT:
		t.to = pdastack_get_start(&thread->stack);
		popped = pdastack_state_pop(&thread->stack, PDA_NODE_SA, &is_empty);

		int reduction_symbol = action->reduction;
		if (action->flags & ACTION_FLAG_IDENTITY) {
			reduction_symbol = prev.token.symbol;
		}
		
		Token reduction;
		if (is_empty) {
			if (reduction_symbol != '\0') {
				//sentinel("Accept pop fail");
				printf("FTH %p: SA pop fail\n", thread);
				exit(1);
			}
			// Assume self accepting symbol.
			// TODO: Maybe it would be safer to have a different
			// action? A start-accept double action?
			reduction = (Token) {
				t.token.index,
				0,
				reduction_symbol
			};
		} else {
			reduction = (Token) {
				popped.token.index,
				t.token.index - popped.token.index,
				reduction_symbol
			};
		}
		t.reduction = reduction;
		break;
	}
	return t;
}

static Transition _partials(Transition transition, FsmThread *thread, Transition prev) {
	Action *action = transition.action;
	Transition t = transition;
	switch(action->type) {
	case ACTION_PARTIAL:
		// pop symbol (reduction)
		pdastack_pop(&thread->stack);

		// TODO: backtracking should happen naturally on errors (if necessary)
		t.to = t.from;
		break;
	default:
		// On any action, except partial itself to avoid infinite loops
		if(t.to && t.to->flags == STATE_FLAG_PARTIAL) {
			int symbol = fsm_get_symbol_id(thread->fsm, nzs("__partial"));
			Token token = {
				t.token.index,
				0,
				symbol
			};
			// TODO: avoid other possible infinite loops, we should
			// not push each time we pass through a particulr state
			// TODO: PDA_NODE_REDUCTION should be named _SYMBOL so
			// it  can be used both for reductions and other
			// pseudo-symbols
			pdastack_state_push(&thread->stack, (PDANode) {
				.type = PDA_NODE_REDUCTION,
				.state = NULL,
				.token = token
			});
		}
		break;
	}
	return t;
}

static Transition _backtrack(Transition transition, FsmThread *thread) {
	Transition t = transition;
	char alt_path = transition.path + 1;
	Action *alt_action = state_get_path_transition(transition.from, transition.token.symbol, alt_path);

	if(alt_action) {
		pdastack_state_push(&thread->stack, (PDANode) {
			.type = PDA_NODE_BACKTRACK,
			.state = transition.from,
			.token = { transition.token.index, 0, 0 },
			.path = transition.path
		});
	} else if (transition.action->type == ACTION_ERROR) {
		bool is_empty;
		PDANode popped = pdastack_state_pop(&thread->stack, PDA_NODE_BACKTRACK, &is_empty);
		if(!is_empty) {
			// TODO: Review the backtrack and path variables.
			// The only problem here: it is not true that we are
			// transitioning to this state with this symbol. This
			// symbol transitioned to error, now we are reseting 
			// the world. Maybe we should reify that.
			// TODO: Communicate we have backtracked!!!!
			t.to = popped.state;
			t.token.index = popped.token.index;
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

	int symbol;
	Token popped = { 0, 0, 0 };
	if(pdastack_has_reduction(&thread->stack)) {
		PDANode top = pdastack_top(&thread->stack);
		popped = top.token;
		symbol = popped.symbol;
	} else {
		symbol = token->symbol;
	}

	action = state_get_path_transition(prev.to, symbol, path);

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
	next.popped = popped;
	next.path = path;
	next.backtrack = 0;

	// Reset path
	thread->path = 0;
	return next;
}

// TODO: Add tests for stack and state manipulators (we are mostly testing
// transitions for now.
void fsm_thread_apply(FsmThread *thread, Transition transition)
{
	// TODO: Consider converting backtracking stack into a loop pipe.
	// Other features, such as shift/reduce stack could also be turned 
	// into pipes. This implies pipes can have state, maybe stored in the 
	// context structure?
	// We should consider which things change and which don't at each step
	Transition t = transition;
	// Order matters, should BACKTRACK nodes be combined with others into
	// a single stack node with multiple uses?
	t = _backtrack(t, thread);
	t = _pop_reductions(t, thread);
	t = _shift_reduce(t, thread);
	t = _start_accept(t, thread, thread->transition);
	t = _partials(t, thread, thread->transition);
	t = _switch_mode(t, thread);

	thread->transition = t;
}


Transition fsm_thread_cycle(FsmThread *thread, const Token token)
{
	Transition transition;

	transition = fsm_thread_match(thread, &token);
	_trace_transition(transition, thread->transition);

	fsm_thread_apply(thread, transition);
	transition = thread->transition;

	return transition;
}

int fsm_thread_notify(FsmThread *thread, Transition transition)
{
	return listener_notify(&thread->pipe, &transition);
}
