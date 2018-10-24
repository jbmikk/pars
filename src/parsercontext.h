#ifndef PARSERCONTEXT_H
#define PARSERCONTEXT_H

#include "parser.h"
#include "fsmthread.h"
#include "input.h"
#include "listener.h"
#include "ast.h"
#include "astbuilder.h"

typedef struct ParserContext {
	Parser *parser;
	Listener parse_setup_lexer;
	Listener parse_setup_fsm;
	Listener parse_start;
	Listener parse_loop;
	Listener parse_end;
	Listener parse_error;
	Listener lexer_pipe;
	Listener parser_pipe;
	FsmThread thread;
	FsmThread lexer_thread;
	Ast *ast;
	AstBuilder ast_builder;
	Input input;
	Input proxy_input;
} ParserContext;

void parser_context_init(ParserContext *context, Parser *parser);
void parser_context_dispose(ParserContext *context);
void parser_context_set_source(ParserContext *context, Source *source);
void parser_context_set_ast(ParserContext *context, Ast *ast);
void parser_context_set_source_ast(ParserContext *context, Ast *ast);
int parser_context_execute(ParserContext *context);

#endif //PARSERCONTEXT_H
