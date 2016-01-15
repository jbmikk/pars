#ifndef FSM_H
#define FSM_H

#include "structs.h"
#include "event.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_CONTEXT_SHIFT 3
#define ACTION_TYPE_ACCEPT 4
#define ACTION_TYPE_ERROR 5

#define EVENT_REDUCE 1
#define EVENT_CONTEXT_SHIFT 2

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
} NonTerminal;

typedef struct _Fsm {
	State *start;
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

typedef struct _Session {
    State *current;
	unsigned int index;
    Stack stack;
	EventListener listener;
} Session;

typedef struct _FsmArgs{
	int symbol;
	unsigned int index;
	unsigned int length;
} FsmArgs;


void session_init(Session *session);
void session_push(Session *session);
void session_pop(Session *session);

void fsm_init(Fsm *fsm);
void fsm_dispose(Fsm *fsm);
NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length);
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm);
void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length);
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
Session *fsm_start_session(Fsm *fsm);
Session *session_set_listener(Session *session, EventListener(listener));
void session_match(Session *session, int symbol, unsigned int index);
State *session_test(Session *session, int symbol, unsigned int index);

#endif //FSM_H
