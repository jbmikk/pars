#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symbols.h"
#include "fsm.h"
#include "ast.h"

typedef struct _Parser {
	Lexer lexer;
	SymbolTable table;
	Fsm fsm;
	FsmHandler handler;
	void (*lexer_handler)(Lexer *lexer);
} Parser;

int parser_execute(Parser *parser, Ast *ast, Input *input);

#endif //PARSER_H
