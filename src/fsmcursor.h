#ifndef FSMCURSOR_H
#define FSMCURSOR_H

#include "fsm.h"

#define REF_PENDING 0
#define REF_SOLVED 1

typedef struct _FsmCursor {
	Fsm *fsm;
	Action *current;
	SNode *stack;
	SNode *continuations;
	Symbol *last_symbol;
	NonTerminal *last_non_terminal;
} FsmCursor;

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm);
void fsm_cursor_dispose(FsmCursor *cur);


// Rule declaration functions

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_end(FsmCursor *cur);
void fsm_cursor_group_start(FsmCursor *cur);
void fsm_cursor_group_end(FsmCursor *cur);
void fsm_cursor_loop_group_start(FsmCursor *cur);
void fsm_cursor_loop_group_end(FsmCursor *cur);
void fsm_cursor_or(FsmCursor *cur);
void fsm_cursor_terminal(FsmCursor *cur, int symbol);
void fsm_cursor_nonterminal(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_done(FsmCursor *cur, int eof_symbol);


// Internal functions?

Action *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length);

#endif //FSMCURSOR_H
