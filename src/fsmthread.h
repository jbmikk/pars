#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "token.h"
#include "stack.h"
#include "listener.h"
#include "continuation.h"


#define FSM_THREAD_NODE_MODE 0
#define FSM_THREAD_NODE_BACKTRACK 1
#define FSM_THREAD_NODE_SR 2
#define FSM_THREAD_NODE_SA 3

typedef struct FsmThreadNode {
	char type;
	State *state;
	int index;
	char path;
} FsmThreadNode;

DEFINE(Stack, FsmThreadNode, FsmThreadNode, fsmthreadnode);


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
	Transition transition;
	Listener pipe;
	char path;
} FsmThread;


void fsm_thread_init(FsmThread *thread, Fsm *fsm, Listener pipe);
void fsm_thread_dispose(FsmThread *thread);

int fsm_thread_start(FsmThread *thread);
int fsm_thread_fake_start(FsmThread *thread, State *state);
Transition fsm_thread_match(FsmThread *thread, const Token *token);
void fsm_thread_apply(FsmThread *thread, Transition transition);
Continuation fsm_thread_cycle(FsmThread *thread, const Token token);
bool fsm_thread_stack_is_empty(FsmThread *thread);

#endif //FSM_THREAD_H
