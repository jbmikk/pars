#include "fsm.h"

#include "cmemory.h"
#include "cradixtree.h"

#define STATE_INIT(V, T) (\
        (V).type = (T)\
    )

void _symbol_to_buffer(unsigned char *buffer, unsigned int *size, int symbol)
{
    int remainder = symbol;
    int i;

    for (i = 0; i < sizeof(int); i++) {
        buffer[i] = remainder & 0xFF;
        remainder >>= 8;
        if(remainder == 0)
        {
            i++;
            break;
        }
    }
    *size = i;
}

void fsm_init(Fsm *fsm)
{
    NODE_INIT(fsm->rules, 0, 0, NULL);
}

void fsm_dispose(Fsm *fsm)
{
    //TODO
}

void frag_init(Frag *frag)
{
    State *begin = c_new(State, 1);
    State *final = c_new(State, 1);
    STATE_INIT(*begin, ACTION_TYPE_SHIFT);
    NODE_INIT(begin->next, 0, 0, NULL);
    STATE_INIT(*final, ACTION_TYPE_SHIFT);
    NODE_INIT(final->next, 0, 0, NULL);
    frag->begin = begin;
    frag->current = begin;
    frag->final = final;
}

Frag *fsm_get_frag(Fsm *fsm, unsigned char *name, int length)
{
    Frag *frag = c_radix_tree_get(&fsm->rules, name, length);
    if(frag == NULL) {
        frag = c_new(Frag, 1);
        frag_init(frag);
        c_radix_tree_set(&fsm->rules, name, length, frag);
    }
    return frag;
}

Frag *fsm_set_start(Fsm *fsm, unsigned char *name, int length)
{
    Frag *frag = fsm_get_frag(fsm, name, length);
    fsm->start = frag->begin;
}

void frag_add_action(Frag *frag, int symbol, int action)
{
    unsigned char buffer[sizeof(int)];
    unsigned int size;
    _symbol_to_buffer(buffer, &size, symbol);

    State *state = c_new(State, 1);
    STATE_INIT(*state, action);

    c_radix_tree_set(&frag->current->next, buffer, size, state);
    if(action == ACTION_TYPE_SHIFT)
        frag->current = state;
}

Session *fsm_start_session(Fsm *fsm)
{
    Session *session = c_new(Session, 1);
    session->current = fsm->start;
    return session;
}

void session_match(Session *session, int symbol)
{
    unsigned char buffer[sizeof(int)];
    unsigned int size;
    _symbol_to_buffer(buffer, &size, symbol);

    State *state = c_radix_tree_get(&session->current->next, buffer, size);
    switch(state->type)
    {
        case ACTION_TYPE_ACCEPT:
        case ACTION_TYPE_SHIFT:
            session->current = state;
            break;
        case ACTION_TYPE_REDUCE:
            //TODO: stack
            break;
        default:
            break;
    }
}
