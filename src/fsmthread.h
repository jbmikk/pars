#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "token.h"
#include "stack.h"

#define FSM_THREAD_OK 0
#define FSM_THREAD_ERROR 1

DEFINE_STACK(State *, State, state);

typedef struct FsmThreadNode {
	State *state;
	int index;
	struct FsmThreadNode *next;
} FsmThreadNode;

typedef struct StateStack {
	FsmThreadNode *top;
} StateStack;


typedef struct _FsmHandler {
	void *target;
	void (*drop)(void *target, const Token *token);
	void (*shift)(void *target, const Token *token);
	void (*reduce)(void *target, const Token *token);
	void (*accept)(void *target, const Token *token);
} FsmHandler;

typedef struct _FsmThread {
	Fsm *fsm;
	int status;
	State *current;
	StateStack stack;
	StackState mode_stack;
	FsmHandler handler;
} FsmThread;

#define NULL_HANDLER ((FsmHandler){NULL, NULL, NULL, NULL})

void fsm_thread_init(FsmThread *thread, Fsm *fsm);
void fsm_thread_dispose(FsmThread *thread);

int fsm_thread_start(FsmThread *thread);
Action *fsm_thread_match(FsmThread *thread, const Token *token);
Action *fsm_thread_test(FsmThread *thread, const Token *token);

#endif //FSM_THREAD_H
