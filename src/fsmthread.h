#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "token.h"
#include "pdastack.h"
#include "listener.h"
#include "transition.h"

typedef struct _FsmThread {
	Fsm *fsm;
	PDAStack stack;
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
Transition fsm_thread_cycle(FsmThread *thread, const Token token);
bool fsm_thread_stack_is_empty(FsmThread *thread);
int fsm_thread_notify(FsmThread *thread, Transition transition);

#endif //FSM_THREAD_H
