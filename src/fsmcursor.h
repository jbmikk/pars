#ifndef FSMCURSOR_H
#define FSMCURSOR_H

#include "fsm.h"

typedef struct _FsmFrame {
	State *start;
	State *continuation;
	struct _FsmFrame *next;
} FsmFrame;

typedef struct _FsmCursor {
	Fsm *fsm;
	Action *action;
	State *state;
	FsmFrame *stack;
	Symbol *last_symbol;
	Nonterminal *last_nonterminal;
} FsmCursor;

void fsm_cursor_init(FsmCursor *cur, Fsm *fsm);
void fsm_cursor_dispose(FsmCursor *cur);


// Rule declaration functions

void fsm_cursor_define(FsmCursor *cur, char *name, int length);
void fsm_cursor_end(FsmCursor *cur);
void fsm_cursor_group_start(FsmCursor *cur);
void fsm_cursor_group_end(FsmCursor *cur);
void fsm_cursor_loop_group_start(FsmCursor *cur);
void fsm_cursor_loop_group_end(FsmCursor *cur);
void fsm_cursor_option_group_start(FsmCursor *cur);
void fsm_cursor_option_group_end(FsmCursor *cur);
void fsm_cursor_or(FsmCursor *cur);
void fsm_cursor_terminal(FsmCursor *cur, int symbol);
void fsm_cursor_nonterminal(FsmCursor *cur, char *name, int length);
void fsm_cursor_done(FsmCursor *cur, int eof_symbol);

#endif //FSMCURSOR_H
