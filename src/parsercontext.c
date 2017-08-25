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
	context->parse_end = parser->parse_end;
	context->parse_end.object = context;
	context->parse_error = parser->parse_error;
	context->parse_error.object = context;

	fsm_thread_init(&context->thread, &parser->fsm);
	fsm_thread_init(&context->lexer_thread, &parser->lexer_fsm);
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

static int _parse_linear_input(ParserContext *context)
{
	Token token;
	token_init(&token, 0, 0, 0);

	do {
		input_next_token(context->input, &token, &token);
		fsm_thread_match(&context->lexer_thread, &token);

		check(
			context->lexer_thread.status != FSM_THREAD_ERROR,
			"Lexer error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

		check(
			context->thread.status != FSM_THREAD_ERROR,
			"Parser error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

	} while(token.symbol != L_EOF);

	return 0;
error:
	return -1;
}

int parser_context_execute(ParserContext *context)
{
	listener_notify(&context->parse_setup_lexer, NULL);
	listener_notify(&context->parse_setup_fsm, NULL);
	listener_notify(&context->parse_start, NULL);

	fsm_thread_start(&context->thread);
	fsm_thread_start(&context->lexer_thread);

	int error = _parse_linear_input(context);
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
