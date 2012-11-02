#ifndef FSM_H
#define FSM_H

#include "cstruct.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_CONTEXT_SHIFT 3
#define ACTION_TYPE_ACCEPT 4

typedef struct _State {
    char type;
    int reduction;
    CNode next;
} State;

typedef struct _Frag {
    State *begin;
    State *current;
    State *final;
} Frag;

typedef struct _Fsm {
    State *start;
    CNode rules;
} Fsm;

typedef struct _SNode {
    State *state;
    struct _SNode *next;
} SNode;

typedef struct _Stack {
    SNode *top;
} Stack;

typedef struct _Session {
    State *current;
    Stack stack;
} Session;

void stack_init(Stack *stack);
void stack_push(Stack *stack, State *state);
State *stack_pop(Stack *stack);

void fsm_init(Fsm *fsm);
void fsm_dispose(Fsm *fsm);
Frag *fsm_get_frag(Fsm *fsm, unsigned char *name, int length);
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);

void frag_init(Frag *frag);
void frag_add_accept(Frag *frag, int symbol);
void frag_add_shift(Frag *frag, int symbol);
void frag_add_context_shift(Frag *frag, int symbol);
void frag_add_followset(Frag *frag, State *state);
void frag_add_reduce(Frag *frag, int symbol, int reduction);
Frag *fsm_set_start(Fsm *fsm, unsigned char *name, int length, int symbol);
Session *fsm_start_session(Fsm *fsm);

#endif //FSM_H
