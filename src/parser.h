#ifndef PARSER_H
#define PARSER_H

#include "fsm.h"

typedef struct ParserContext ParserContext;

typedef void (ContextBuilder)(ParserContext *context);

typedef struct Parser {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;
	ContextBuilder *build_context;
} Parser;

void parser_init(Parser *parser);
void parser_dispose(Parser *parser);

#endif //PARSER_H
