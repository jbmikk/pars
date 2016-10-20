#ifndef FSM_H
#define FSM_H

#include "structs.h"
#include "symbols.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_CONTEXT_SHIFT 3
#define ACTION_TYPE_ACCEPT 4
#define ACTION_TYPE_ERROR 5

typedef struct _State {
	Node actions;
} State;

typedef struct _Action {
	char type;
	int reduction;
	State *state;
} Action;

typedef struct _NonTerminal {
	Action *start;
	Action *end;
	char unsolved_returns;
	char unsolved_invokes;
	struct _Reference *parent_refs;
} NonTerminal;

typedef struct _Reference {
	Action *action;
	Symbol *symbol;
	NonTerminal *non_terminal;
	char return_status;
	char invoke_status;
	struct _Reference *next;
} Reference;

typedef struct _Fsm {
	Action *start;
	Action error;
	SymbolTable *table;
} Fsm;

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
void session_dispose(Session *session);
void session_push(Session *session);
void session_pop(Session *session);

void fsm_init(Fsm *fsm, SymbolTable *table);
void fsm_dispose(Fsm *fsm);

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length);
Symbol *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length);

Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length);
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);
int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length);

Session *session_set_handler(Session *session, FsmHandler handler, void *target);
void session_match(Session *session, int symbol, unsigned int index, unsigned int length);
Action *session_test(Session *session, int symbol, unsigned int index, unsigned int length);

void _state_init(State *state);
void _action_init(Action *action, char type, int reduction, State *state);

#endif //FSM_H
