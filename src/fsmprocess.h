#ifndef FSM_THREAD_H
#define FSM_THREAD_H

#include "fsm.h"
#include "token.h"

#define FSM_THREAD_OK 0
#define FSM_THREAD_ERROR 1

typedef struct _FsmProcessNode {
	State *state;
	int index;
	struct _FsmProcessNode *next;
} FsmProcessNode;

typedef struct _StateStack {
	FsmProcessNode *top;
} StateStack;

typedef struct _ModeNode {
	State *state;
	struct _ModeNode *next;
} ModeNode;

typedef struct _ModeStack {
	ModeNode *top;
} ModeStack;

typedef struct _FsmHandler {
	void *target;
	void (*drop)(void *target, const Token *token);
	void (*shift)(void *target, const Token *token);
	void (*reduce)(void *target, const Token *token);
	void (*accept)(void *target, const Token *token);
} FsmHandler;

typedef struct _FsmProcess {
	Fsm *fsm;
	int status;
	State *current;
	StateStack stack;
	ModeStack mode_stack;
	FsmHandler handler;
} FsmProcess;

#define NULL_HANDLER ((FsmHandler){NULL, NULL, NULL, NULL})

void fsm_process_init(FsmProcess *process, Fsm *fsm);
void fsm_process_dispose(FsmProcess *process);

int fsm_process_start(FsmProcess *process);
Action *fsm_process_match(FsmProcess *process, const Token *token);
Action *fsm_process_test(FsmProcess *process, const Token *token);

#endif //FSM_THREAD_H
