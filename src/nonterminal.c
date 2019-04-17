#include "fsm.h"

#include <stdlib.h>

#include "fsmtrace.h"

void nonterminal_init(Nonterminal *nonterminal)
{
	bmap_ref_init(&nonterminal->refs);
	nonterminal->status = NONTERMINAL_CLEAR;
	nonterminal->start = NULL;
	nonterminal->end = NULL;
	nonterminal->sibling_end = NULL;
	nonterminal->mode = 0;
	nonterminal->pushes_mode = 0;
	nonterminal->pops_mode = 0;
	nonterminal->type = NONTERMINAL_TYPE_DEFAULT;
}

void nonterminal_add_reference(Nonterminal *nonterminal, State *state, Symbol *symbol)
{
	Reference *ref = malloc(sizeof(Reference));
	ref->state = state;
	ref->to_state = NULL;
	//TODO: Is it used?
	ref->symbol = symbol;
	ref->status = REF_PENDING;
	ref->nonterminal = NULL;
	ref->cont = NULL;

	//TODO: is ref key ok?
	bmap_ref_insert(&nonterminal->refs, (intptr_t)ref, ref);
	nonterminal->status |= NONTERMINAL_RETURN_REF;

	//Set end state status if exists
	if(nonterminal->end) {
		nonterminal->end->status |= STATE_RETURN_REF;
	}
}

void nonterminal_dispose(Nonterminal *nonterminal)
{
	//Delete all references
	Reference *ref;
	BMapCursorReference rcursor;

	bmap_cursor_ref_init(&rcursor, &nonterminal->refs);
	while(bmap_cursor_ref_next(&rcursor)) {
		ref = bmap_cursor_ref_current(&rcursor)->ref;
		free(ref);
	}
	bmap_cursor_ref_dispose(&rcursor);

	bmap_ref_dispose(&nonterminal->refs);
}

int nonterminal_solve_references(Nonterminal *nt) {
	int result = REF_RESULT_SOLVED;
	BMapCursorReference rcursor;
	Reference *ref;

	if(nt->status == NONTERMINAL_CLEAR) {
		goto end;
	}

	bmap_cursor_ref_init(&rcursor, &nt->refs);
	while(bmap_cursor_ref_next(&rcursor)) {
		ref = bmap_cursor_ref_current(&rcursor)->ref;
		reference_solve_return_set(ref, nt, &result);
	}
	bmap_cursor_ref_dispose(&rcursor);

	if(result == REF_RESULT_SOLVED) {
		nt->status = NONTERMINAL_CLEAR;
		nt->end->status &= ~STATE_RETURN_REF;
		trace_state(
			"state return refs clear",
			nt->end,
			""
		);
	}

end:
	return result;
}
