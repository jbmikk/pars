#include "fsm.h"

#include "cmemory.h"
#include "radixtree.h"
#include "arrays.h"

#include <stdio.h>
#include <stdint.h>

void fsm_init(Fsm *fsm, SymbolTable *table)
{
	//TODO: Get symbol table as parameter
	fsm->table = table;

	symbol_table_add(fsm->table, "__empty", 7);
	
	radix_tree_init(&fsm->nonterminals);

	action_init(&fsm->error, ACTION_ERROR, NULL_SYMBOL, NULL, 0, 0);
	fsm->error.state = c_new(State, 1);
	state_init(fsm->error.state);

	//Accept state
	fsm->accept = c_new(State, 1);
	state_init(fsm->accept);
}

void fsm_get_states(Node *states, State *state)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;

	if(state) {
		//TODO: Should have a separate ptr_to_array function
		int_to_array(buffer, &size, (intptr_t)state);

		Action *in_states = radix_tree_get(states, buffer, size);

		if(!in_states) {
			radix_tree_set(states, buffer, size, state);

			Iterator it;

			//Jump to other states
			Action *ac;
			radix_tree_iterator_init(&it, &(state->actions));
			while((ac = (Action *)radix_tree_iterator_next(&it))) {
				fsm_get_states(states, ac->state);
			}
			radix_tree_iterator_dispose(&it);

			//Jump to references
			Reference *ref;
			radix_tree_iterator_init(&it, &state->refs);
			while((ref = (Reference *)radix_tree_iterator_next(&it))) {
				fsm_get_states(states, ref->to_state);
			}
			radix_tree_iterator_dispose(&it);
		}

	}
}

void fsm_dispose(Fsm *fsm)
{
	Node all_states;

	Nonterminal *nt;
	Iterator it;

	radix_tree_init(&all_states);

	radix_tree_iterator_init(&it, &fsm->nonterminals);
	while((nt = (Nonterminal *)radix_tree_iterator_next(&it))) {
		//Just in case some nonterminal is not reachable through start
		fsm_get_states(&all_states, nt->start);
		nonterminal_dispose(nt);
		c_delete(nt);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&fsm->nonterminals);

	//Make sure accept state is reachable
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;
	int_to_array(buffer, &size, (intptr_t)fsm->accept);
	radix_tree_try_set(&all_states, buffer, size, fsm->accept);

	//Delete all states
	State *st;
	radix_tree_iterator_init(&it, &all_states);
	while((st = (State *)radix_tree_iterator_next(&it))) {
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

Nonterminal *fsm_get_nonterminal_by_id(Fsm *fsm, int symbol)
{
	return (Nonterminal *)radix_tree_get_int(&fsm->nonterminals, symbol);
}

Nonterminal *fsm_get_nonterminal(Fsm *fsm, char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? fsm_get_nonterminal_by_id(fsm, symbol->id): NULL;
}

Nonterminal *fsm_create_nonterminal(Fsm *fsm, char *name, int length)
{
	Nonterminal *nonterminal = fsm_get_nonterminal(fsm, name, length);
	if(!nonterminal) {
		Symbol *symbol = symbol_table_add(fsm->table, name, length);
		nonterminal = c_new(Nonterminal, 1);
		nonterminal_init(nonterminal);
		nonterminal->start = c_new(State, 1);
		state_init(nonterminal->start);
		radix_tree_set_int(&fsm->nonterminals, symbol->id, nonterminal);
		//TODO: Add to nonterminal struct: 
		// * detect circular references.
	}
	return nonterminal;
}

State *fsm_get_state_by_id(Fsm *fsm, int symbol)
{
	return fsm_get_nonterminal_by_id(fsm, symbol)->start;
}

State *fsm_get_state(Fsm *fsm, char *name, int length)
{
	return fsm_get_nonterminal(fsm, name, length)->start;
}

Symbol *fsm_get_symbol(Fsm *fsm, char *name, int length)
{
	return symbol_table_get(fsm->table, name, length);
}

int fsm_get_symbol_id(Fsm *fsm, char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? symbol->id: 0;
}
