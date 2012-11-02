#include "fsm.h"

#include "cmemory.h"
#include "cradixtree.h"

#define STATE_INIT(V, T, R) (\
        (V).type = (T),\
        (V).reduction = (R)\
    )

#define NONE 0
#include <stdio.h>
void stack_init(Stack *stack)
{
    stack->top = NULL;
}

void stack_push(Stack *stack, State *state)
{
    SNode *node = c_new(SNode, 1);
    node->state = state;
    node->next = stack->top;
    stack->top = node;
}

State *stack_pop(Stack *stack)
{
    SNode *top = stack->top;
    State *state = top->state;
    stack->top = top->next;
    c_delete(top);
    return state;
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
    stack_init(&session->stack);
    stack_push(&session->stack, session->current);
    return session;
}

void session_match(Session *session, int symbol)
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
		return;
	}

    switch(state->type)
    {
        case ACTION_TYPE_CONTEXT_SHIFT:
            stack_push(&session->stack, session->current);
            session->current = state;
            break;
        case ACTION_TYPE_ACCEPT:
        case ACTION_TYPE_SHIFT:
            session->current = state;
            break;
        case ACTION_TYPE_REDUCE:
            session->current = stack_pop(&session->stack);
			session_match(session, state->reduction);
            goto rematch; // same as session_match(session, symbol);
            break;
        default:
            break;
    }
}
