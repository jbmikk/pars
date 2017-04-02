#ifndef SESSION_H
#define SESSION_H

#include "fsm.h"
#include "token.h"

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
	void *target;
	void (*shift)(void *target, const Token *token);
	void (*reduce)(void *target, const Token *token);
	void (*accept)(void *target, const Token *token);
} FsmHandler;

typedef struct _Session {
	Fsm *fsm;
	int status;
	State *current;
	Action *last_action;
	unsigned int index;
	Stack stack;
	FsmHandler handler;
} Session;

#define NULL_HANDLER ((FsmHandler){NULL, NULL, NULL})

void session_init(Session *session, Fsm *fsm, FsmHandler handler);
void session_dispose(Session *session);

void session_push(Session *session);
void session_pop(Session *session);

void session_match(Session *session, const Token *token);
Action *session_test(Session *session, const Token *token);

#endif //SESSION_H
