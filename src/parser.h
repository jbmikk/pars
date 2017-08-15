#ifndef PARSER_H
#define PARSER_H

#include "symbols.h"
#include "fsm.h"
#include "ast.h"
#include "fsmthread.h"

typedef struct _Parser {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;
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
int parser_execute(Parser *parser, Ast *ast, Input *input);

#endif //PARSER_H
