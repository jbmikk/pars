#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"
#include <stdio.h>

//# State functions

#ifdef FSM_TRACE
#define trace(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p)--> %-9p] %-13s %c %3i (%3i=%2c)\n", \
		M, \
		T1, \
		T2, \
		T2? ((Action*)T2)->state: NULL, \
		A, \
		(R != 0)? '>': ' ', \
		R, \
		S, (char)S \
	)
#else
#define trace(M, T1, T2, S, A, R)
#endif

void state_init(State *state)
{
	radix_tree_init(&state->actions, 0, 0, NULL);
}

void state_dispose(State *state)
{
	Iterator it;

	//Delete all actions
	Action *ac;
	radix_tree_iterator_init(&it, &state->actions);
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		c_delete(ac);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&state->actions);
}

Action *state_add(State *state, int symbol, int type, int reduction)
{
	Action * action = c_new(Action, 1);
	action_init(action, type, reduction, NULL);

	//TODO: detect duplicates
	radix_tree_set_int(&state->actions, symbol, action);

	if(type == ACTION_CONTEXT_SHIFT) {
		trace("add", state, action, symbol, "context-shift", 0);
	} else if(type == ACTION_SHIFT) {
		trace("add", state, action, symbol, "shift", 0);
	} else if(type == ACTION_REDUCE) {
		trace("add", state, action, symbol, "reduce", 0);
	} else if(type == ACTION_ACCEPT) {
		trace("add", state, action, symbol, "accept", 0);
	} else {
		trace("add", state, action, symbol, "action", 0);
	}
	return action;
}

Action *state_add_buffer(State *state, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	if(action == NULL) {
		action = c_new(Action, 1);
		action_init(action, type, reduction, NULL);
	}

	Action *prev = (Action *)radix_tree_try_set(&state->actions, buffer, size, action);
	if(prev) {
		if(
			prev->type == action->type &&
			prev->reduction == action->reduction
		) {
			trace("dup", state, action, array_to_int(buffer, size), "skip", reduction);
		} else {
			trace("dup", state, action, array_to_int(buffer, size), "conflict", reduction);
			//TODO: add sentinel ?
		}
		c_delete(action);
		action = NULL;
	}
	return action;
}

void state_add_first_set(State *state, State* source)
{
	Action *action, *clone;
	Iterator it;
	radix_tree_iterator_init(&it, &(source->actions));
	while(action = (Action *)radix_tree_iterator_next(&it)) {
		//TODO: Make type for clone a parameter, do not override by
		// default.
		clone = c_new(Action, 1);
		clone->reduction = action->reduction;
		clone->state = action->state;
		clone->type = ACTION_CONTEXT_SHIFT;
		state_add_buffer(state, it.key, it.size, 0, 0, clone);
		trace("add", state, action, array_to_int(it.key, it.size), "first", 0);
	}
	radix_tree_iterator_dispose(&it);
}

void state_add_reduce_follow_set(State *from, State *to, int symbol)
{
	Action *ac;
	Iterator it;

	// Empty transitions should not be cloned.
	// They should be followed recursively to get the whole follow set,
	// otherwise me might loose reductions.
	radix_tree_iterator_init(&it, &(to->actions));
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		state_add_buffer(from, it.key, it.size, ACTION_REDUCE, symbol, NULL);
		trace("add", from, ac, array_to_int(it.key, it.size), "reduce-follow", symbol);
	}
	radix_tree_iterator_dispose(&it);
}



//# Action functions

void action_init(Action *action, char type, int reduction, State *state)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
}

static void action_ensure_state(Action *action)
{
	if(!action->state) {
		action->state = c_new(State, 1);
		state_init(action->state);
	}
}

Action *action_add_buffer(Action *from, unsigned char *buffer, unsigned int size, int type, int reduction, Action *action)
{
	action_ensure_state(from);
	return state_add_buffer(from->state, buffer, size, type, reduction, action);
}

Action *action_add(Action *action, int symbol, int type, int reduction)
{
	action_ensure_state(action);
	return state_add(action->state, symbol, type, reduction);
}

void action_add_first_set(Action *action, State* source)
{
	action_ensure_state(action);
	state_add_first_set(action->state, source);
}

void action_add_reduce_follow_set(Action *from, Action *to, int symbol)
{
	action_ensure_state(from);
	state_add_reduce_follow_set(from->state, to->state, symbol);
}


//# NonTerminal functions

void nonterminal_init(NonTerminal *nonterminal)
{
	nonterminal->parent_refs = NULL;
	nonterminal->unsolved_returns = 0;
	nonterminal->unsolved_invokes = 0;
}

void nonterminal_add_reference(NonTerminal *nonterminal, Action *action, Symbol *symbol, Symbol *from_symbol, NonTerminal *from_nonterminal)
{
	//Create reference from last non terminal to the named non terminal
	Reference *pref = c_new(Reference, 1);
	pref->action = action;
	pref->symbol = from_symbol;
	pref->non_terminal = from_nonterminal;
	pref->invoke_status = REF_PENDING;
	pref->return_status = REF_PENDING;

	//Push the reference on the non terminal
	pref->next = nonterminal->parent_refs;
	nonterminal->parent_refs = pref;
	nonterminal->unsolved_returns++;

	//Only count children at the beginning of a non terminal
	if(pref->action == &pref->non_terminal->start) {
		pref->non_terminal->unsolved_invokes++;
	}

}

void nonterminal_dispose(NonTerminal *nonterminal)
{
	//Delete all references
	Reference *ref = nonterminal->parent_refs;
	while(ref) {
		Reference *pref = ref;
		ref = ref->next;
		c_delete(pref);
	}
}
