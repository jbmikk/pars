#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "output.h"
#include "token.h"
#include "stack.h"

DEFINE_STACK(State *, State, state);

typedef struct FsmThreadNode {
	State *state;
	int index;
} FsmThreadNode;

DEFINE_STACK(FsmThreadNode, FsmThreadNode, fsmthreadnode);

typedef struct _FsmHandler {
	void *target;
	void (*drop)(void *target, const Token *token);
	void (*shift)(void *target, const Token *token);
	void (*reduce)(void *target, const Token *token);
	void (*accept)(void *target, const Token *token);
} FsmHandler;

typedef struct _FsmThread {
	Fsm *fsm;
	State *current;
	StackFsmThreadNode stack;
	StackState mode_stack;
	FsmHandler handler;
	Output *output;
} FsmThread;

#define NULL_HANDLER ((FsmHandler){NULL, NULL, NULL, NULL})

void fsm_thread_init(FsmThread *thread, Fsm *fsm, Output *output);
void fsm_thread_dispose(FsmThread *thread);

int fsm_thread_start(FsmThread *thread);
Action *fsm_thread_match(FsmThread *thread, const Token *token);
Action *fsm_thread_test(FsmThread *thread, const Token *token);

#endif //FSM_THREAD_H
