#include "astlistener.h"

#include "parsercontext.h"

int ast_parse_start(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	ast_init(context->ast, context->input, &context->parser->table);

	//Fsm thread target is the ast
	context->thread.handler.target = context->ast;

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

