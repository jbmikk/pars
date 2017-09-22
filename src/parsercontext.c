#include "parsercontext.h"
#include "dbg.h"

void parser_context_init(ParserContext *context, Parser *parser)
{
	context->input = NULL;
	context->ast = NULL;

	context->parser = parser;
	context->parse_setup_lexer = parser->parse_setup_lexer;
	context->parse_setup_lexer.object = context;
	context->parse_setup_fsm = parser->parse_setup_fsm;
	context->parse_setup_fsm.object = context;
	context->parse_start = parser->parse_start;
	context->parse_start.object = context;
	context->parse_loop = parser->parse_loop;
	context->parse_loop.object = context;
	context->parse_end = parser->parse_end;
	context->parse_end.object = context;
	context->parse_error = parser->parse_error;
	context->parse_error.object = context;

	fsm_process_init(&context->process, &parser->fsm);
	fsm_process_init(&context->lexer_process, &parser->lexer_fsm);
}

void parser_context_dispose(ParserContext *context)
{
	fsm_process_dispose(&context->process);
	fsm_process_dispose(&context->lexer_process);
}

void parser_context_set_input(ParserContext *context, Input *input)
{
	context->input = input;
}

void parser_context_set_ast(ParserContext *context, Ast *ast)
{
	context->ast = ast;
}

void parser_context_set_input_ast(ParserContext *context, Ast *ast)
{
	context->input_ast = ast;
}

int parser_context_execute(ParserContext *context)
{
	listener_notify(&context->parse_setup_lexer, NULL);
	listener_notify(&context->parse_setup_fsm, NULL);
	listener_notify(&context->parse_start, NULL);

	fsm_process_start(&context->process);
	fsm_process_start(&context->lexer_process);

	int error = listener_notify(&context->parse_loop, NULL);
	check(!error, "Error during parse.");

	listener_notify(&context->parse_end, NULL);

	fsm_process_dispose(&context->process);
	fsm_process_dispose(&context->lexer_process);

	return 0;
error:
	//TODO: free
	fsm_process_dispose(&context->process);
	fsm_process_dispose(&context->lexer_process);

	listener_notify(&context->parse_error, NULL);

	return -1;
}
