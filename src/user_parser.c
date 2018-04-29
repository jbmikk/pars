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

static void _user_pipe_token(void *thread, const Token *token)
{
	FsmThread *_thread = (FsmThread *)thread;
	Continuation cont;

	int count = 0;
	Token retry = *token;
	do {
		cont = fsm_thread_match(_thread, &retry);
		fsm_thread_notify(_thread, &cont);

		// TODO: Temporary continuation, it should be in control loop
	} while (!pda_continuation_follow(&cont, token, &retry, &count));
}

int _user_setup_lexer(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->lexer_thread.handler.target = &context->thread;
	context->lexer_thread.handler.drop = NULL;
	context->lexer_thread.handler.shift = NULL;
	context->lexer_thread.handler.reduce = NULL;
	context->lexer_thread.handler.accept = _user_pipe_token;
	return 0;
}

int user_build_parser(Parser *parser)
{
	listener_init(&parser->parse_setup_lexer, _user_setup_lexer, NULL);
	listener_init(&parser->parse_setup_fsm, ast_setup_fsm, NULL);
	listener_init(&parser->parse_start, ast_parse_start, NULL);
	listener_init(&parser->parse_loop, control_loop_linear, NULL);
	listener_init(&parser->parse_end, ast_parse_end, NULL);
	listener_init(&parser->parse_error, ast_parse_error, NULL);

	//TODO: Maybe basic initialization should be separate from specific
	//initialization for different kinds of parsers.
	//TODO: We don't have a lexer for the source yet
	_identity_init_lexer_fsm(&parser->lexer_fsm);
	return 0;
//error:
	//TODO: free

	//return -1;
}
