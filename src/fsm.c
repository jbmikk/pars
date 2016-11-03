#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"

#include <stdio.h>
#include <stdint.h>

#define NONE 0

void fsm_init(Fsm *fsm, SymbolTable *table)
{
	//TODO: Get symbol table as parameter
	fsm->table = table;
	fsm->start = NULL;

	symbol_table_add(fsm->table, "__empty", 7);

	action_init(&fsm->error, ACTION_TYPE_ERROR, NONE, NULL);
	fsm->error.state = c_new(State, 1);
	state_init(fsm->error.state);
}

void _fsm_get_actions(Node *actions, Node *states, Action *action)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	//TODO: Should have a separate ptr_to_array function
	int_to_array(buffer, &size, (intptr_t)action);

	Action *in_actions = radix_tree_get(actions, buffer, size);

	if(!in_actions) {
		radix_tree_set(actions, buffer, size, action);

		State *state = action->state;
		if(state) {
			Action *ac;
			Iterator it;
			radix_tree_iterator_init(&it, &(state->actions));
			while(ac = (Action *)radix_tree_iterator_next(&it)) {
				_fsm_get_actions(actions, states, ac);
			}
			radix_tree_iterator_dispose(&it);

			//Add state to states
			int_to_array(buffer, &size, (intptr_t)state);
			State *in_states = radix_tree_get(states, buffer, size);
			if(!in_states) {
				radix_tree_set(states, buffer, size, state);
			}
		}
	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_actions;
	Node all_states;
	Symbol *symbol;
	NonTerminal *nt;
	Iterator it;

	radix_tree_init(&all_actions, 0, 0, NULL);
	radix_tree_init(&all_states, 0, 0, NULL);

	//Get all actions reachable through the starting action
	if(fsm->start) {
		_fsm_get_actions(&all_actions, &all_states, fsm->start);
	}

	radix_tree_iterator_init(&it, &fsm->table->symbols);
	while(symbol = (Symbol *)radix_tree_iterator_next(&it)) {
		//Get all actions reachable through other rules
		nt = (NonTerminal *)symbol->data;

		if(!nt) {
			//Some symbols may not have non terminals
			//TODO: Should we have a separate non terminals array?
			continue;
		}

		_fsm_get_actions(&all_actions, &all_states, nt->start);
		Reference *ref = nt->parent_refs;
		while(ref) {
			Reference *pref = ref;
			ref = ref->next;
			c_delete(pref);
		}
		c_delete(nt);
		//TODO: Symbol table may live longer than fsm, makes sense?
		symbol->data = NULL;
	}
	radix_tree_iterator_dispose(&it);

	//Delete all actions
	Action *ac;
	radix_tree_iterator_init(&it, &all_actions);
	while(ac = (Action *)radix_tree_iterator_next(&it)) {
		c_delete(ac);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&all_actions);

	//Delete all states
	State *st;
	radix_tree_iterator_init(&it, &all_states);
	while(st = (State *)radix_tree_iterator_next(&it)) {
		state_dispose(st);
		c_delete(st);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&all_states);

	//Delete error state
	radix_tree_dispose(&fsm->error.state->actions);
	c_delete(fsm->error.state);

	fsm->table = NULL;
}

NonTerminal *fsm_get_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? (NonTerminal *)symbol->data: NULL;
}

Symbol *fsm_create_non_terminal(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_add(fsm->table, name, length);
	NonTerminal *non_terminal;
	Action *action;
	if(!symbol->data) {
		non_terminal = c_new(NonTerminal, 1);
		action = c_new(Action, 1);
		action_init(action, ACTION_TYPE_SHIFT, NONE, NULL);
		non_terminal->start = action;
		non_terminal->end = action;
		non_terminal->parent_refs = NULL;
		non_terminal->unsolved_returns = 0;
		non_terminal->unsolved_invokes = 0;
		symbol->data = non_terminal;
		//TODO: Add to non_terminal struct: 
		// * detect circular references.
	}
	return symbol;
}


Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start;
}

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start->state;
}

int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? symbol->id: 0;
}
