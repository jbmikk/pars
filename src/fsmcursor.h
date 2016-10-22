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
Action *fsm_cursor_set_start(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_move(FsmCursor *cur, unsigned char *name, int length);

void fsm_cursor_define(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_group_start(FsmCursor *cur);
void fsm_cursor_group_end(FsmCursor *cur);
void fsm_cursor_loop_group_start(FsmCursor *cur);
void fsm_cursor_loop_group_end(FsmCursor *cur);
void fsm_cursor_or(FsmCursor *cur);
void fsm_cursor_end(FsmCursor *cur);

void fsm_cursor_add_reference(FsmCursor *cur, unsigned char *name, int length);
void fsm_cursor_push(FsmCursor *cur);
void fsm_cursor_pop(FsmCursor *cur);
void fsm_cursor_pop_discard(FsmCursor *cur);
void fsm_cursor_reset(FsmCursor *cur);
void fsm_cursor_push_continuation(FsmCursor *cur);
void fsm_cursor_push_new_continuation(FsmCursor *cur);
State *fsm_cursor_pop_continuation(FsmCursor *cur);
void fsm_cursor_join_continuation(FsmCursor *cur);
void fsm_cursor_dispose(FsmCursor *cur);

void fsm_cursor_done(FsmCursor *cur, int eof_symbol);
void fsm_cursor_add_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_context_shift(FsmCursor *cur, int symbol);
void fsm_cursor_add_first_set(FsmCursor *cur, State *state);
void fsm_cursor_add_reduce(FsmCursor *cur, int symbol, int reduction);
void fsm_cursor_add_empty(FsmCursor *cur);
FsmCursor *fsm_set_start(Fsm *fsm, unsigned char *name, int length);

#endif //FSMCURSOR_H
