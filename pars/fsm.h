#ifndef FSM_H
#define FSM_H

#include "structs.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_CONTEXT_SHIFT 3
#define ACTION_TYPE_ACCEPT 4
#define ACTION_TYPE_ERROR 5

typedef struct _State {
	char type;
	int reduction;
	Node next;
} State;

typedef struct _NonTerminal {
	State *start;
	int symbol;
	char *name;
	int length;
	struct _Reference *child_refs;
	struct _Reference *parent_refs;
} NonTerminal;

typedef struct _Reference {
	State *state;
	NonTerminal *non_terminal;
	struct _Reference *next;
} Reference;

typedef struct _Fsm {
	State *start;
	State error;
	Node rules;
	int symbol_base;
} Fsm;

typedef struct _FsmCursor {
	Fsm *fsm;
	State *current;
	SNode *stack;
	SNode *followset_stack;
	NonTerminal *last_non_terminal;
} FsmCursor;

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
	State *current;
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
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm);
void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_push(FsmCursor *cur);
void fsm_cursor_pop(FsmCursor *cur);
void fsm_cursor_push_followset(FsmCursor *cur);
State *fsm_cursor_pop_followset(FsmCursor *cur);
void fsm_cursor_dispose(FsmCursor *cur);

void fsm_cursor_add_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_followset(FsmCursor *cur, State *state);
void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction);
FsmCursor *fsm_set_start(Fsm *fsm, unsigned char *name, int length, int symbol);
Session *session_set_handler(Session *session, FsmHandler handler, void *target);
void session_match(Session *session, int symbol, unsigned int index, unsigned int length);
State *session_test(Session *session, int symbol, unsigned int index, unsigned int length);

#endif //FSM_H
