#ifndef FSM_H
#define FSM_H

#include "cstruct.h"

#define ACTION_TYPE_START 0
#define ACTION_TYPE_SHIFT 1
#define ACTION_TYPE_REDUCE 2
#define ACTION_TYPE_ACCEPT 3

typedef struct _State {
    char type;
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

typedef struct _Session{
    State *current;
} Session;

void fsm_init(Fsm *fsm);
void fsm_dispose(Fsm *fsm);
Frag *fsm_get_frag(Fsm *fsm, unsigned char *name, int length);

void frag_init(Frag *frag);
void frag_add_action(Frag *frag, int symbol, int action);
Frag *fsm_set_start(Fsm *fsm, unsigned char *name, int length);
Session *fsm_start_session(Fsm *fsm);

#endif //FSM_H
