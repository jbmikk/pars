#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "fsm.h"
#include "ast.h"

typedef struct _Parser {
	Lexer lexer;
	Fsm fsm;
	EVENT_HANDLER(handler);
} Parser;

int parser_execute(Parser *parser, Ast *ast, Input *input);

#endif //PARSER_H
