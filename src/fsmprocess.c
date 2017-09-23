#include "fsmprocess.h"

void fsm_process_init(FsmProcess *process, Fsm *fsm)
{
	process->fsm = fsm;
	process->handler = NULL_HANDLER;
	process->status = 0;
	fsm_thread_init(&process->thread, fsm);
}

void fsm_process_dispose(FsmProcess *process)
{
	fsm_thread_dispose(&process->thread);
}

int fsm_process_start(FsmProcess *process)
{
	process->thread.handler = process->handler;
	return fsm_thread_start(&process->thread);
}

Action *fsm_process_match(FsmProcess *process, const Token *token)
{
	Action *action = fsm_thread_match(&process->thread, token);
	//TODO: Remove status from threads?
	process->status = process->thread.status;
	return action;
}

Action *fsm_process_test(FsmProcess *process, const Token *token)
{
	return fsm_thread_test(&process->thread, token);
}
