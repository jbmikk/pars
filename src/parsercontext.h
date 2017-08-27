#ifndef PARSERCONTEXT_H
#define PARSERCONTEXT_H

#include "parser.h"
#include "listener.h"
#include "ast.h"

typedef struct _ParserContext {
	Parser *parser;
	Listener parse_setup_lexer;
	Listener parse_setup_fsm;
	Listener parse_start;
	Listener parse_loop;
	Listener parse_end;
	Listener parse_error;
	FsmThread thread;
	FsmThread lexer_thread;
	Ast *ast;
	Input *input;
	Ast *input_ast;
} ParserContext;

void parser_context_init(ParserContext *context, Parser *parser);
void parser_context_dispose(ParserContext *context);
void parser_context_set_input(ParserContext *context, Input *input);
void parser_context_set_ast(ParserContext *context, Ast *ast);
void parser_context_set_input_ast(ParserContext *context, Ast *ast);
int parser_context_execute(ParserContext *context);

#endif //PARSERCONTEXT_H
