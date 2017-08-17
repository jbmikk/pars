#ifndef PARSER_H
#define PARSER_H

#include "fsm.h"
#include "listener.h"
#include "fsmthread.h"

typedef struct _Parser {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;
	Listener parse_start;
	Listener parse_end;
	Listener parse_error;
	FsmThread proto_fsm_thread;
	FsmThread proto_lexer_fsm_thread;
} Parser;

void parser_init(Parser *parser);
void parser_dispose(Parser *parser);
void parser_setup_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
);
void parser_setup_lexer_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
);

#endif //PARSER_H
