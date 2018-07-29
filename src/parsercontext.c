#include "parsercontext.h"
#include "dbg.h"

void parser_context_init(ParserContext *context, Parser *parser)
{
	context->input = NULL;
	context->ast = NULL;

	context->parser = parser;
	parser->build_context(context);

	fsm_thread_init(&context->thread, &parser->fsm, context->parser_pipe);
	fsm_thread_init(&context->lexer_thread, &parser->lexer_fsm, context->lexer_pipe);
}

void parser_context_dispose(ParserContext *context)
{
	fsm_thread_dispose(&context->thread);
	fsm_thread_dispose(&context->lexer_thread);
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

	fsm_thread_start(&context->thread);
	fsm_thread_start(&context->lexer_thread);

	int error = listener_notify(&context->parse_loop, NULL);
	check(!error, "Error during parse.");

	listener_notify(&context->parse_end, NULL);

	fsm_thread_dispose(&context->thread);
	fsm_thread_dispose(&context->lexer_thread);

	return 0;
error:
	//TODO: free
	fsm_thread_dispose(&context->thread);
	fsm_thread_dispose(&context->lexer_thread);

	listener_notify(&context->parse_error, NULL);

	return -1;
}
