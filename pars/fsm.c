#include "fsm.h"

#include "cmemory.h"
#include "cradixtree.h"

#define STATE_INIT(V, T, R) (\
        (V).type = (T),\
        (V).reduction = (R)\
    )

#define NONE 0
#include <stdio.h>

#ifdef FSM_TRACE
#define trace(M, T, S, A) printf("\n\ttrace: %i %s '%c' (%i) -> %s", (unsigned int)T, M, (char)S, S, A);
#else
#define trace(M, T, S, A)
#endif

void session_init(Session *session)
{
	session->stack.top = NULL;
}

void session_push(Session *session)
{
	SNode *node = c_new(SNode, 1);
	node->state = session->current;
	node->index = session->index;
	node->next = session->stack.top;
	session->stack.top = node;
}

void session_pop(Session *session)
{
	SNode *top = session->stack.top;
	session->current = top->state;
	session->index = top->index;
	session->stack.top = top->next;
	c_delete(top);
}


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
    STATE_INIT(*begin, ACTION_TYPE_SHIFT, NONE);
    NODE_INIT(begin->next, 0, 0, NULL);
    STATE_INIT(*final, ACTION_TYPE_SHIFT, NONE);
    NODE_INIT(final->next, 0, 0, NULL);
    frag->begin = begin;
    frag->current = begin;
    frag->final = final;
}

void frag_rewind(Frag *frag)
{
    frag->current = frag->begin;
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

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
    Frag *frag = c_radix_tree_get(&fsm->rules, name, length);
    return frag->begin;
}

Frag *fsm_set_start(Fsm *fsm, unsigned char *name, int length, int symbol)
{
    Frag *frag = fsm_get_frag(fsm, name, length);
    fsm->start = frag->begin;//should not use frag->begin
    frag->current = frag->begin; //should not have to do this
    frag_add_accept(frag, symbol);
}

State *_frag_add_action_buffer(Frag *frag, unsigned char *buffer, unsigned int size, int action, int reduction, State *state)
{
    if(state == NULL)
    {
        state = c_new(State, 1);
        STATE_INIT(*state, action, reduction);
		//trace("init", state, action, "");
    }

    c_radix_tree_set(&frag->current->next, buffer, size, state);
    return state;
}

State *_frag_add_action(Frag *frag, int symbol, int action, int reduction, State *state)
{
    unsigned char buffer[sizeof(int)];
    unsigned int size;
    _symbol_to_buffer(buffer, &size, symbol);

    return _frag_add_action_buffer(frag, buffer, size, action, reduction, state);
}

void frag_add_accept(Frag *frag, int symbol)
{
    State *state = _frag_add_action(frag, symbol, ACTION_TYPE_ACCEPT, NONE, NULL);
    frag->current = state;
}

void frag_add_shift(Frag *frag, int symbol)
{
    State *state = _frag_add_action(frag, symbol, ACTION_TYPE_SHIFT, NONE, NULL);
    frag->current = state;
}

void frag_add_context_shift(Frag *frag, int symbol)
{
    State *state = _frag_add_action(frag, symbol, ACTION_TYPE_CONTEXT_SHIFT, NONE, NULL);
    frag->current = state;
}

void frag_add_followset(Frag *frag, State *state)
{
	State *s;
	CIterator it;
    c_radix_tree_iterator_init(&(state->next), &it);
	while((s = (State *)c_radix_tree_iterator_next(&(state->next), &it)) != NULL) {
		_frag_add_action_buffer(frag, it.key, it.size, 0, 0, s);
	}
    c_radix_tree_iterator_dispose(&(state->next), &it);
}

void frag_add_reduce(Frag *frag, int symbol, int reduction)
{
    _frag_add_action(frag, symbol, ACTION_TYPE_REDUCE, reduction, NULL);
}

Session *fsm_start_session(Fsm *fsm)
{
    Session *session = c_new(Session, 1);
    session->current = fsm->start;
	session_init(session);
	session_push(session);
    return session;
}


State *session_test(Session *session, int symbol, int index)
{
    unsigned char buffer[sizeof(int)];
    unsigned int size;
    _symbol_to_buffer(buffer, &size, symbol);
	State *state;
	State *prev;

    state = c_radix_tree_get(&session->current->next, buffer, size);

    switch(state->type)
    {
        case ACTION_TYPE_CONTEXT_SHIFT:
			trace("test", session->current, symbol, "context shift");
            break;
        case ACTION_TYPE_ACCEPT:
			trace("test", session->current, symbol, "accept");
            break;
        case ACTION_TYPE_SHIFT:
			trace("test", session->current, symbol, "shift");
            break;
        case ACTION_TYPE_REDUCE:
			trace("test", session->current, symbol, "reduce");
            break;
        default:
            break;
    }
	return state;
}

void session_match(Session *session, int symbol, int index)
{
    unsigned char buffer[sizeof(int)];
    unsigned int size;
	int prev_symbol = 0;
	State *state;
    _symbol_to_buffer(buffer, &size, symbol);

rematch:
    state = c_radix_tree_get(&session->current->next, buffer, size);

	if(state == NULL)
	{
		//Should jump to error state or throw exception?
		if(session->current->type != ACTION_TYPE_ACCEPT)
			trace("match", session->current, symbol, "error");
		return;
	}

    switch(state->type)
    {
        case ACTION_TYPE_CONTEXT_SHIFT:
			trace("match", session->current, symbol, "context shift");
			session_push(session);
            session->current = state;
            break;
        case ACTION_TYPE_ACCEPT:
			trace("match", session->current, symbol, "accept");
            session->current = state;
            break;
        case ACTION_TYPE_SHIFT:
			trace("match", session->current, symbol, "shift");
            session->current = state;
            break;
        case ACTION_TYPE_REDUCE:
			trace("match", session->current, symbol, "reduce");
			session_pop(session);
			session_match(session, state->reduction, session->index);
            goto rematch; // same as session_match(session, symbol);
            break;
        default:
            break;
    }
}
