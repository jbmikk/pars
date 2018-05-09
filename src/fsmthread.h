#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "output.h"
#include "token.h"
#include "stack.h"
#include "listener.h"

DEFINE_STACK(State *, State, state);

typedef struct FsmThreadNode {
	State *state;
	int index;
} FsmThreadNode;

DEFINE_STACK(FsmThreadNode, FsmThreadNode, fsmthreadnode);

typedef struct _FsmThread {
	Fsm *fsm;
	State *current;
	StackFsmThreadNode stack;
	StackState mode_stack;
	Output *output;
} FsmThread;

typedef struct Transition {
	Action *action;
	Token token;
} Transition;


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Output *output);
void fsm_thread_dispose(FsmThread *thread);

int fsm_thread_start(FsmThread *thread);
Transition fsm_thread_match(FsmThread *thread, const Token *token);
Transition fsm_pda_loop(FsmThread *thread, const Token token, Listener listener);

#endif //FSM_THREAD_H
