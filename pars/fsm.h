#ifndef FSM_H
#define FSM_H

#include "cstruct.h"
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
Frag *fsm_get_frag(Fsm *fsm, unsigned char *name, int length);
State *fsm_get_state(Fsm *fsm, unsigned char *name, int length);

void frag_init(Frag *frag);
void frag_rewind(Frag *frag);
void frag_add_accept(Frag *frag, int symbol);
void frag_add_shift(Frag *frag, int symbol);
void frag_add_context_shift(Frag *frag, int symbol);
void frag_add_followset(Frag *frag, State *state);
void frag_add_reduce(Frag *frag, int symbol, int reduction);
Frag *fsm_set_start(Fsm *fsm, unsigned char *name, int length, int symbol);
Session *fsm_start_session(Fsm *fsm);
Session *session_set_listener(Session *session, EventListener(listener));
void session_match(Session *session, int symbol, unsigned int index);
State *session_test(Session *session, int symbol, unsigned int index);

#endif //FSM_H
