#include "astlistener.h"

#include "parsercontext.h"

int ast_setup_fsm(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->thread.handler.target = context->ast;
	context->thread.handler.drop = ast_append;
	context->thread.handler.shift = ast_open;
	context->thread.handler.reduce = ast_close;
	context->thread.handler.accept = NULL;

	return 0;
}

int ast_parse_start(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	ast_init(context->ast, context->input, &context->parser->table);

	return 0;
}

int ast_parse_end(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	//TODO: validate errors
	ast_done(context->ast);
	return 0;
}

int ast_parse_error(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	ast_dispose(context->ast);
	return 0;
}

