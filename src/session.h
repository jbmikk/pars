#ifndef SESSION_H
#define SESSION_H

#include "fsm.h"

#define SESSION_OK 0
#define SESSION_ERROR 1

typedef struct _SessionNode {
	State *state;
	int index;
	struct _SessionNode *next;
} SessionNode;

typedef struct _Stack {
	SessionNode *top;
} Stack;

typedef struct _FsmHandler {
	void (*context_shift)(void *target, unsigned int index, unsigned int length, int symbol);
	void (*reduce)(void *target, unsigned int index, unsigned int length, int symbol);
} FsmHandler;

typedef struct _Session {
	Fsm *fsm;
	int status;
	State *current;
	Action *last_action;
	unsigned int index;
	unsigned int length;
	Stack stack;
	FsmHandler handler;
	void *target;
} Session;

void session_init(Session *session, Fsm *fsm);
void session_dispose(Session *session);

Session *session_set_handler(Session *session, FsmHandler handler, void *target);

void session_push(Session *session);
void session_pop(Session *session);

void session_match(Session *session, int symbol, unsigned int index, unsigned int length);
Action *session_test(Session *session, int symbol, unsigned int index, unsigned int length);

#endif //SESSION_H
