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
#define ACTION_POP 7
#define ACTION_POP_SHIFT 8
#define ACTION_PARTIAL 9

#define ACTION_FLAG_RANGE 1
#define ACTION_FLAG_MODE_PUSH 2
#define ACTION_FLAG_MODE_POP 4

// Used for identity accept
#define ACTION_FLAG_IDENTITY 8

// TODO: Rename to SYMBOL_NONE?
#define NONE 0

#define REF_PENDING 0
#define REF_SOLVED 1

#define REF_RESULT_SOLVED 0
#define REF_RESULT_PENDING 1
#define REF_RESULT_CHANGED 2

#define REF_TYPE_DEFAULT 0
#define REF_TYPE_SHIFT 1
#define REF_TYPE_START 2
#define REF_TYPE_COPY 3
#define REF_TYPE_COPY_MERGE 4

#define REF_STRATEGY_MERGE 0
#define REF_STRATEGY_SPLIT 1

#define STATE_CLEAR 0
#define STATE_INVOKE_REF 1
#define STATE_RETURN_REF 2

#define STATE_EVENT_NONE 0
#define STATE_EVENT_IN 1
#define STATE_EVENT_OUT 2

#define NONTERMINAL_CLEAR 0
#define NONTERMINAL_RETURN_REF 1

#define NONTERMINAL_TYPE_DEFAULT 0
#define NONTERMINAL_TYPE_IDENTITY 1

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
	char events;
};

typedef struct Nonterminal {
	State *start;
	State *end;
	State *sibling_end;
	BMapReference refs;
	//mode == 0 means no parent mode.
	int mode;
	//TODO: Only necessary for setting accept action flags.
	int pushes_mode;
	int pops_mode;
	char status;
	char type;
} Nonterminal;

// Invoking refernces are from a concrete state to a symbol (to be resolved to
// states). Returning references are from a symbol (to be resolved to an end
// state) to a concrete state.
typedef struct Reference {
	State *state;
	State *to_state;
	// Used for: copy
	State *cont;
	// Used for: copy
	Nonterminal *nonterminal;
	// Used for: nothing
	Symbol *symbol;
	char type;
	char strategy;
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
Symbol *fsm_get_symbol(Fsm *fsm, char *name, int length);
Symbol *fsm_get_symbol_by_id(Fsm *fsm, int id);
int fsm_get_symbol_id(Fsm *fsm, char *name, int length);

//# State functions

void state_init(State *state);
void state_get_states(State *state, BMapState *states);
void state_add_reference(State *state, char type, char strategy, Symbol *symbol, State *to_state);
void state_add_reference_with_cont(State *state, char type, char strategy, Symbol *symbol, State *to_state, Nonterminal *nt, State *cont);
Action *state_get_transition(State *state, int symbol);
Action *state_get_path_transition(State *state, int symbol, int path);
void state_dispose(State *state);
Action *state_add(State *from, int symbol, int type, int reduction);
Action *state_add_range(State *state, Range range, int type, int reduction);
Action *state_append_action(State *state, int symbol, Action *action);
int state_solve_references(State *state);
bool state_all_ready(State *state, BMapState *walked);
void state_print(State *state);


//# Action functions

void action_init(Action *action, char type, int reduction, State *state, char flags, int end_symbol);
int action_compare(Action a1, Action a2);
Action *action_add(Action *from, int symbol, int type, int reduction);


//# Nonterminal functions

void nonterminal_init(Nonterminal *nonterminal);
int nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol);
int nonterminal_solve_references(Nonterminal *nt);
void nonterminal_dispose(Nonterminal *nonterminal);


// # Reference functions

void reference_solve_first_set(Reference *ref, int *result);
void reference_solve_return_set(Reference *ref, Nonterminal *nt, int *result);


#endif //FSM_H
