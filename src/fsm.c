#include "fsm.h"

#include "cmemory.h"
#include "rtree.h"
#include "arrays.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


DEFINE_BMAP_FUNCTIONS(int, Nonterminal *, Nonterminal, nonterminal, IMPLEMENTATION)

DEFINE_BMAP_FUNCTIONS(intptr_t, State *, State, state, IMPLEMENTATION)


void fsm_init(Fsm *fsm, SymbolTable *table)
{
	fsm->table = table;
	bmap_nonterminal_init(&fsm->nonterminals);
}

void fsm_get_states(BMapState *states, State *state)
{
	if(state) {
		BMapEntryState *in_states = bmap_state_get(states, (intptr_t)state);

		if(!in_states) {
			bmap_state_insert(states, (intptr_t)state, state);

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
	BMapState all_states;

	Nonterminal *nt;
	BMapCursorNonterminal cursor;

	bmap_state_init(&all_states);

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
	BMapCursorState scursor;
	bmap_cursor_state_init(&scursor, &all_states);
	while(bmap_cursor_state_next(&scursor)) {
		st = bmap_cursor_state_current(&scursor)->state;
		state_dispose(st);
		free(st);
	}
	bmap_cursor_state_dispose(&scursor);
	bmap_state_dispose(&all_states);

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
