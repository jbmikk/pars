#include "astlistener.h"

#include "parsercontext.h"
#include "astbuilder.h"

int ast_setup_fsm(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->thread.handler.target = &context->ast_builder;
	context->thread.handler.drop = ast_builder_append;
	context->thread.handler.shift = ast_builder_open;
	context->thread.handler.reduce = ast_builder_close;
	context->thread.handler.accept = NULL;

	return 0;
}

int ast_parse_start(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	ast_init(context->ast, context->input, &context->parser->table);
	ast_builder_init(&context->ast_builder, context->ast);

	return 0;
}

int ast_parse_end(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	//TODO: validate errors
	ast_builder_done(&context->ast_builder);
	return 0;
}

int ast_parse_error(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	ast_builder_dispose(&context->ast_builder);
	ast_dispose(context->ast);
	return 0;
}

