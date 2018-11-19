#ifndef FSMBUILDER_H
#define FSMBUILDER_H

#include "fsm.h"

typedef struct FsmFrame {
	State *start;
	State *continuation;
	struct FsmFrame *next;
} FsmFrame;

typedef struct FsmBuilder {
	Fsm *fsm;
	Action *action;
	State *state;
	FsmFrame *stack;
	Symbol *last_symbol;
	int current_mode;
	Nonterminal *last_nonterminal;
} FsmBuilder;

void fsm_builder_init(FsmBuilder *builder, Fsm *fsm);
void fsm_builder_dispose(FsmBuilder *builder);


// Rule declaration functions

void fsm_builder_define(FsmBuilder *builder, char *name, int length);
void fsm_builder_end(FsmBuilder *builder);

void fsm_builder_set_mode(FsmBuilder *builder, char *name, int length);
void fsm_builder_mode_push(FsmBuilder *builder, char *name, int length);
void fsm_builder_mode_pop(FsmBuilder *builder);

void fsm_builder_group_start(FsmBuilder *builder);
void fsm_builder_group_end(FsmBuilder *builder);
void fsm_builder_loop_group_start(FsmBuilder *builder);
void fsm_builder_loop_group_end(FsmBuilder *builder);
void fsm_builder_option_group_start(FsmBuilder *builder);
void fsm_builder_option_group_end(FsmBuilder *builder);
void fsm_builder_or(FsmBuilder *builder);
void fsm_builder_terminal(FsmBuilder *builder, int symbol);
void fsm_builder_terminal_range(FsmBuilder *builder, Range range);
void fsm_builder_any(FsmBuilder *builder);
void fsm_builder_nonterminal(FsmBuilder *builder, char *name, int length);
void fsm_builder_done(FsmBuilder *builder, int eof_symbol);
void fsm_builder_lexer_done(FsmBuilder *builder, int eof_symbol);
void fsm_builder_lexer_default_input(FsmBuilder *builder);

#endif //FSMBUILDER_H
