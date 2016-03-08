#ifndef FSM_H
#define FSM_H

#include "structs.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_CONTEXT_SHIFT 3
#define ACTION_TYPE_ACCEPT 4
#define ACTION_TYPE_ERROR 5

typedef struct _Action {
	char type;
	int reduction;
	Node actions;
} Action;

typedef struct _NonTerminal {
	Action *start;
	Action *end;
	int symbol;
	char *name;
	int length;
	struct _Reference *child_refs;
	struct _Reference *parent_refs;
} NonTerminal;

typedef struct _Reference {
	Action *action;
	NonTerminal *non_terminal;
	struct _Reference *next;
} Reference;

typedef struct _Fsm {
	Action *start;
	Action error;
	Node rules;
	int symbol_base;
} Fsm;

typedef struct _FsmCursor {
	Fsm *fsm;
	Action *current;
	Action *previous;
	int last_symbol;
	SNode *stack;
	SNode *continuations;
	NonTerminal *last_non_terminal;
} FsmCursor;

typedef struct _SessionNode {
	Action *action;
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
	Action *current;
	unsigned int index;
	unsigned int length;
	Stack stack;
	FsmHandler handler;
	void *target;
} Session;


void session_init(Session *session, Fsm *fsm);
void session_push(Session *session);
void session_pop(Session *session);

void fsm_init(Fsm *fsm);
void fsm_dispose(Fsm *fsm);
NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length);
Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length);

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm);
void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_push(FsmCursor *cur);
void fsm_cursor_pop(FsmCursor *cur);
void fsm_cursor_reset(FsmCursor *cur);
void fsm_cursor_push_continuation(FsmCursor *cur);
void fsm_cursor_push_new_continuation(FsmCursor *cur);
Action *fsm_cursor_pop_continuation(FsmCursor *cur);
void fsm_cursor_join_continuation(FsmCursor *cur);
void fsm_cursor_follow_continuation(FsmCursor *cur);
void fsm_cursor_dispose(FsmCursor *cur);

void fsm_cursor_add_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_followset(FsmCursor *cur, Action *action);
void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction);
FsmCursor *fsm_set_start(Fsm *fsm, unsigned char *name, int length, int symbol);
Session *session_set_handler(Session *session, FsmHandler handler, void *target);
void session_match(Session *session, int symbol, unsigned int index, unsigned int length);
Action *session_test(Session *session, int symbol, unsigned int index, unsigned int length);

#endif //FSM_H
