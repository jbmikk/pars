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

int parser_execute(Parser *parser, Ast *ast, Input *input);

#endif //PARSER_H
