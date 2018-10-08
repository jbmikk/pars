#ifndef FSM_H
#define FSM_H

#include "symbols.h"
#include "rtree.h"

#include <string.h>

#define nzs(S) (S), (strlen(S))

#define ACTION_START 0
#define ACTION_DROP 1
#define ACTION_REDUCE 2
#define ACTION_SHIFT 3
#define ACTION_ACCEPT 4
#define ACTION_ERROR 5
#define ACTION_EMPTY 6

#define ACTION_FLAG_RANGE 1
#define ACTION_FLAG_MODE_PUSH 2
#define ACTION_FLAG_MODE_POP 4

#define REF_PENDING 0
#define REF_SOLVED 1

#define STATE_CLEAR 0
#define STATE_INVOKE_REF 1
#define STATE_RETURN_REF 2

#define NONTERMINAL_CLEAR 0
#define NONTERMINAL_RETURN_REF 1

typedef struct State State;

typedef struct Action {
	char type;
	char flags;
	int reduction;
	int mode;
	int end_symbol;
	State *state;
} Action;

typedef struct Reference Reference;

DEFINE(BMap, int, Action, Action, action)

DEFINE(BMap, intptr_t, Reference *, Reference, ref)

struct State {
	BMapAction actions;
	BMapReference refs;
	char status;
};

typedef struct Nonterminal {
	State *start;
	State *end;
	BMapReference refs;
	//mode == 0 means no parent mode.
	int mode;
	//TODO: Only necessary for setting accept action flags.
	int pushes_mode;
	int pops_mode;
	char status;
} Nonterminal;

// Invoking refernces are from a concrete state to a symbol (to be resolved to
// states). Returning references are from a symbol (to be resolved to an end
// state) to a concrete state.
typedef struct Reference {
	State *state;
	State *to_state;
	Symbol *symbol;
	char type;
	char status;
} Reference;

DEFINE(BMap, int, Nonterminal *, Nonterminal, nonterminal)

DEFINE(BMap, intptr_t, State *, State, state)

typedef struct Fsm {
	SymbolTable *table;
	BMapNonterminal nonterminals;
} Fsm;


PROTOTYPES(BMap, int, Action, Action, action)

PROTOTYPES(BMap, int, Nonterminal *, Nonterminal, nonterminal)

PROTOTYPES(BMap, intptr_t, State *, State, state)

PROTOTYPES(BMap, intptr_t, Reference*, Reference, ref)

void fsm_init(Fsm *fsm, SymbolTable *table);
void fsm_dispose(Fsm *fsm);

Nonterminal *fsm_get_nonterminal(Fsm *fsm, char *name, int length);
Nonterminal *fsm_create_nonterminal(Fsm *fsm, char *name, int length);

State *fsm_get_state(Fsm *fsm, char *name, int length);
State *fsm_get_state_by_id(Fsm *fsm, int symbol);
//TODO: Is this signature ok?
void fsm_get_states(BMapState *states, State *state);
Symbol *fsm_get_symbol(Fsm *fsm, char *name, int length);
Symbol *fsm_get_symbol_by_id(Fsm *fsm, int id);
int fsm_get_symbol_id(Fsm *fsm, char *name, int length);

//# State functions

void state_init(State *state);
void state_add_reference(State *state, Symbol *symbol, State *to_state);
Action *state_get_transition(State *state, int symbol);
void state_dispose(State *state);
Action *state_add(State *from, int symbol, int type, int reduction);
Action *state_add_range(State *state, Range range, int type, int reduction);
Action *state_append_action(State *state, int symbol, Action *action);
void state_add_reduce_follow_set(State *from, State *to, int symbol);


//# Action functions

void action_init(Action *action, char type, int reduction, State *state, char flags, int end_symbol);
Action *action_add(Action *from, int symbol, int type, int reduction);


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal);
void nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol);
void nonterminal_dispose(Nonterminal *nonterminal);


// # Reference functions

void reference_solve_first_set(Reference *ref, int *unsolved);
void reference_solve_return_set(Reference *ref, Nonterminal *nt, int *unsolved);


#endif //FSM_H
