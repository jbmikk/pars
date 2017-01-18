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

#define NULL_SYMBOL 0

#define REF_PENDING 0
#define REF_SOLVED 1

#define STATE_CLEAR 0
#define STATE_INVOKE_REF 1
#define STATE_RETURN_REF 2

#define NONTERMINAL_CLEAR 0
#define NONTERMINAL_RETURN_REF 1

typedef struct _State {
	Node actions;
	Node refs;
	char status;
} State;

typedef struct _Action {
	char type;
	int reduction;
	State *state;
} Action;

typedef struct _Nonterminal {
	State *start;
	State *end;
	Node refs;
	char status;
} Nonterminal;

// Invoking refernces are from a concrete state to a symbol (to be resolved to
// states). Returning references are from a symbol (to be resolved to an end
// state) to a concrete state.
typedef struct _Reference {
	State *state;
	Symbol *symbol;
	char type;
	char status;
} Reference;

typedef struct _Fsm {
	State *start;
	Action error;
	State *accept;
	SymbolTable *table;
} Fsm;


void fsm_init(Fsm *fsm, SymbolTable *table);
void fsm_dispose(Fsm *fsm);

Nonterminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length);
Symbol *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length);

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);
void fsm_get_states(Node *states, State *state);
int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length);

//# State functions

void state_init(State *state);
void state_add_reference(State *state, Symbol *symbol);
void state_dispose(State *state);
Action *state_add(State *from, int symbol, int type, int reduction);
void state_add_first_set(State *from, State* state);
void state_add_reduce_follow_set(State *from, State *to, int symbol);


//# Action functions

void action_init(Action *action, char type, int reduction, State *state);
Action *action_add(Action *from, int symbol, int type, int reduction);
void action_add_first_set(Action *from, State* state);
void action_add_reduce_follow_set(Action *from, Action *to, int symbol);


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal);
void nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol);
void nonterminal_dispose(Nonterminal *nonterminal);

#endif //FSM_H
