#include "fsm.h"

#include "cmemory.h"
#include "rtree.h"
#include "arrays.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

DEFINE_BMAP_FUNCTIONS(int, Nonterminal *, Nonterminal, nonterminal, IMPLEMENTATION)

void fsm_init(Fsm *fsm, SymbolTable *table)
{
	fsm->table = table;
	bmap_nonterminal_init(&fsm->nonterminals);
}

void fsm_get_states(RTree *states, State *state)
{
	unsigned char buffer[sizeof(intptr_t)];
	unsigned int size;

	if(state) {
		//TODO: Should have a separate ptr_to_array function
		int_to_array(buffer, &size, (intptr_t)state);

		Action *in_states = rtree_get(states, buffer, size);

		if(!in_states) {
			rtree_set(states, buffer, size, state);

			//Jump to other states
			BMapCursorAction cursor;
			Action *ac;
			bmap_cursor_action_init(&cursor, &state->actions);
			while(bmap_cursor_action_next(&cursor)) {
				ac = bmap_cursor_action_current(&cursor)->action;
				fsm_get_states(states, ac->state);
			}
			bmap_cursor_action_dispose(&cursor);

			//Jump to references
			Iterator it;
			Reference *ref;
			rtree_iterator_init(&it, &state->refs);
			while((ref = (Reference *)rtree_iterator_next(&it))) {
				fsm_get_states(states, ref->to_state);
			}
			rtree_iterator_dispose(&it);
		}

	}
}

void fsm_dispose(Fsm *fsm)
{
	RTree all_states;

	Nonterminal *nt;
	Iterator it;
	BMapCursorNonterminal cursor;

	rtree_init(&all_states);

	bmap_cursor_nonterminal_init(&cursor, &fsm->nonterminals);
	while(bmap_cursor_nonterminal_next(&cursor)) {
		nt = bmap_cursor_nonterminal_current(&cursor)->nonterminal;
		//Just in case some nonterminal is not reachable through start
		fsm_get_states(&all_states, nt->start);
		nonterminal_dispose(nt);
		free(nt);
	}
	bmap_cursor_nonterminal_dispose(&cursor);

	bmap_nonterminal_dispose(&fsm->nonterminals);

	//Delete all states
	State *st;
	rtree_iterator_init(&it, &all_states);
	while((st = (State *)rtree_iterator_next(&it))) {
		state_dispose(st);
		free(st);
	}
	rtree_iterator_dispose(&it);
	rtree_dispose(&all_states);

	fsm->table = NULL;
}

Nonterminal *fsm_get_nonterminal_by_id(Fsm *fsm, int symbol)
{
	BMapEntryNonterminal *entry = bmap_nonterminal_get(&fsm->nonterminals, symbol);
	return entry? entry->nonterminal: NULL;
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
		nonterminal = malloc(sizeof(Nonterminal));
		nonterminal_init(nonterminal);
		nonterminal->start = malloc(sizeof(State));
		state_init(nonterminal->start);
		bmap_nonterminal_insert(&fsm->nonterminals, symbol->id, nonterminal);
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

Symbol *fsm_get_symbol_by_id(Fsm *fsm, int id)
{
	return symbol_table_get_by_id(fsm->table, id);
}

int fsm_get_symbol_id(Fsm *fsm, char *name, int length)
{
	Symbol *symbol = symbol_table_get(fsm->table, name, length);
	return symbol? symbol->id: 0;
}
