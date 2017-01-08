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
	action_init(&fsm->start, ACTION_ERROR, NONE, NULL);

	symbol_table_add(fsm->table, "__empty", 7);

	action_init(&fsm->error, ACTION_ERROR, NONE, NULL);
	fsm->error.state = c_new(State, 1);
	state_init(fsm->error.state);

	//Accept state
	fsm->accept = c_new(State, 1);
	state_init(fsm->accept);
}

void _fsm_get_states(Node *states, Action *action)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	State *state = action->state;

	if(state) {
		//TODO: Should have a separate ptr_to_array function
		int_to_array(buffer, &size, (intptr_t)state);

		Action *in_states = radix_tree_get(states, buffer, size);

		if(!in_states) {
			radix_tree_set(states, buffer, size, state);

			Action *ac;
			Iterator it;
			radix_tree_iterator_init(&it, &(state->actions));
			while(ac = (Action *)radix_tree_iterator_next(&it)) {
				_fsm_get_states(states, ac);
			}
			radix_tree_iterator_dispose(&it);
		}
	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_states;
	Symbol *symbol;
	NonTerminal *nt;
	Iterator it;

	radix_tree_init(&all_states, 0, 0, NULL);

	//Get all actions reachable through the starting action
	_fsm_get_states(&all_states, &fsm->start);

	radix_tree_iterator_init(&it, &fsm->table->symbols);
	while(symbol = (Symbol *)radix_tree_iterator_next(&it)) {
		//Get all actions reachable through other rules
		nt = (NonTerminal *)symbol->data;

		if(!nt) {
			//Some symbols may not have non terminals
			//TODO: Should we have a separate non terminals array?
			continue;
		}

		_fsm_get_states(&all_states, &nt->start);
		nonterminal_dispose(nt);
		c_delete(nt);
		//TODO: Symbol table may live longer than fsm, makes sense?
		symbol->data = NULL;
	}
	radix_tree_iterator_dispose(&it);

	//Make sure accept state is reachable
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	int_to_array(buffer, &size, (intptr_t)fsm->accept);
	radix_tree_try_set(&all_states, buffer, size, fsm->accept);

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
	state_dispose(fsm->error.state);
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
	if(!symbol->data) {
		non_terminal = c_new(NonTerminal, 1);
		action_init(&non_terminal->start, ACTION_SHIFT, NONE, NULL);
		non_terminal->end = &non_terminal->start;
		nonterminal_init(non_terminal);
		symbol->data = non_terminal;
		//TODO: Add to non_terminal struct: 
		// * detect circular references.
	}
	return symbol;
}


Action *fsm_get_action(Fsm *fsm, unsigned char *name, int length)
{
	return &fsm_get_non_terminal(fsm, name, length)->start;
}

State *fsm_get_state(Fsm *fsm, unsigned char *name, int length)
{
	return fsm_get_non_terminal(fsm, name, length)->start.state;
}

int fsm_get_symbol(Fsm *fsm, unsigned char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? symbol->id: 0;
}
