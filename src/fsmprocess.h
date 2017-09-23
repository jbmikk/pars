#ifndef FSM_PROCESS_H
#define FSM_PROCESS_H

#include "fsm.h"
#include "token.h"
#include "fsmthread.h"

#endif //FSM_PROCESS_H

typedef struct _FsmProcess {
	Fsm *fsm;
	int status;
	FsmThread thread;
	FsmHandler handler;
} FsmProcess;

void fsm_process_init(FsmProcess *process, Fsm *fsm);
void fsm_process_dispose(FsmProcess *process);

int fsm_process_start(FsmProcess *process);
Action *fsm_process_match(FsmProcess *process, const Token *token);
Action *fsm_process_test(FsmProcess *process, const Token *token);
