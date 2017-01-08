#ifndef FSM_H
#define FSM_H

#include "structs.h"
#include "symbols.h"

#define ACTION_START 0
#define ACTION_SHIFT 1
#define ACTION_REDUCE 2
#define ACTION_CONTEXT_SHIFT 3
#define ACTION_ACCEPT 4
#define ACTION_ERROR 5
#define ACTION_EMPTY 6

#define REF_PENDING 0
#define REF_SOLVED 1

typedef struct _State {
	Node actions;
} State;

typedef struct _Action {
	char type;
	int reduction;
	State *state;
} Action;

typedef struct _NonTerminal {
	Action start;
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
	Action start;
	Action error;
	State *accept;
	SymbolTable *table;
} Fsm;


void fsm_init(Fsm *fsm, SymbolTable *table);
void fsm_dispose(Fsm *fsm);

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length);
Symbol *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length);

Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length);
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);
int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length);


//# State functions

void state_init(State *state);
void state_dispose(State *state);
Action *state_add_buffer(State *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action);
Action *state_add(State *from, int symbol, int type, int reduction);
void state_add_first_set(State *from, State* state);
void state_add_reduce_follow_set(State *from, State *to, int symbol);


//# Action functions

void action_init(Action *action, char type, int reduction, State *state);
Action *action_add_buffer(Action *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action);
Action *action_add(Action *from, int symbol, int type, int reduction);
void action_add_first_set(Action *from, State* state);
void action_add_reduce_follow_set(Action *from, Action *to, int symbol);


//# NonTerminal functions

void nonterminal_init(NonTerminal *nonterminal);
void nonterminal_add_reference(NonTerminal *nonterminal, Action *action, Symbol *symbol, Symbol *from_symbol, NonTerminal *from_nonterminal);
void nonterminal_dispose(NonTerminal *nonterminal);

#endif //FSM_H
