#ifndef PARSER_H
#define PARSER_H

#include "symbols.h"
#include "fsm.h"
#include "ast.h"
#include "session.h"

typedef struct _Parser {
	SymbolTable table;
	Fsm fsm;
	FsmHandler handler;
	Fsm lexer_fsm;
	FsmHandler lexer_handler;
} Parser;

void parser_init(Parser *parser);
void parser_dispose(Parser *parser);
void parser_set_handlers(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
);
void parser_set_lexer_handlers(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
);
int parser_execute(Parser *parser, Ast *ast, Input *input);

#endif //PARSER_H
