#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "token.h"
#include "stack.h"
#include "listener.h"
#include "continuation.h"

DEFINE(Stack, State *, State, state);

typedef struct FsmThreadNode {
	State *state;
	int index;
} FsmThreadNode;

DEFINE(Stack, FsmThreadNode, FsmThreadNode, fsmthreadnode);

typedef struct BacktrackNode {
	State *state;
	int index;
	char path;
} BacktrackNode;

DEFINE(Stack, BacktrackNode, BacktrackNode, backtracknode);

typedef struct Transition {
	State *from;
	State *to;
	Action *action;
	char path;
	Token token;
	Token reduction;
	// TODO: If actions weren't pointers we could build a BT action.
	char backtrack;
} Transition;

typedef struct _FsmThread {
	Fsm *fsm;
	State *start;
	StackFsmThreadNode stack;
	StackState mode_stack;
	StackBacktrackNode btstack;
	Transition transition;
	Listener pipe;
	char path;
} FsmThread;


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Listener pipe);
void fsm_thread_dispose(FsmThread *thread);

int fsm_thread_start(FsmThread *thread);
Transition fsm_thread_match(FsmThread *thread, const Token *token);
void fsm_thread_apply(FsmThread *thread, Transition transition);
Continuation fsm_thread_cycle(FsmThread *thread, const Token token);

#endif //FSM_THREAD_H
