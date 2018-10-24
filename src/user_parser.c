#include "user_parser.h"
#include "parsercontext.h"
#include "astlistener.h"
#include "controlloop.h"
#include "fsmbuilder.h"

static void _identity_init_lexer_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	fsm_builder_set_mode(&builder, nzs(".default"));

	fsm_builder_lexer_done(&builder, L_EOF);
	fsm_builder_lexer_default_input(&builder);

	fsm_builder_dispose(&builder);
}

int user_lexer_pipe(void *_context, void *_tran)
{
	ParserContext *context = (ParserContext *)_context;
	Transition *tran = (Transition *)_tran;

	if(tran->action->type != ACTION_ACCEPT) {
		return 0;
	}
	Token token = tran->token;

	Continuation cont = { .error = 0 };
	cont = input_loop(&context->proxy_input, &context->thread, token);

	return cont.error;
}

void user_build_parser_context(ParserContext *context)
{
	listener_init(&context->parse_setup_lexer, NULL, context);
	listener_init(&context->parse_setup_fsm, NULL, context);
	listener_init(&context->parse_start, ast_parse_start, context);
	listener_init(&context->parse_loop, control_loop_linear, context);
	listener_init(&context->parse_end, ast_parse_end, context);
	listener_init(&context->parse_error, ast_parse_error, context);

	listener_init(&context->lexer_pipe, user_lexer_pipe, context);
	listener_init(&context->parser_pipe, ast_parser_pipe, context);
}

int user_build_parser(Parser *parser)
{

	//TODO: Maybe basic initialization should be separate from specific
	//initialization for different kinds of parsers.
	//TODO: We don't have a lexer for the source yet
	_identity_init_lexer_fsm(&parser->lexer_fsm);

	parser->build_context = user_build_parser_context;

	return 0;
//error:
	//TODO: free

	//return -1;
}
