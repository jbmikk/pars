#include "astlistener.h"

#include "parsercontext.h"
#include "astbuilder.h"

int ast_parser_transition(void *_context, void *_cont)
{
	ParserContext *context = (ParserContext *)_context;
	Continuation *cont = (Continuation *)_cont;

	switch(cont->action->type) {
	case ACTION_DROP:
		ast_builder_drop(&context->ast_builder, &cont->token);
		break;
	case ACTION_SHIFT:
		ast_builder_shift(&context->ast_builder, &cont->token);
		break;
	case ACTION_REDUCE:
		ast_builder_reduce(&context->ast_builder, &cont->token);
		break;
	}
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

