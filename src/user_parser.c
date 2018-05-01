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

int user_lexer_transition(void *_context, void *_cont)
{
	ParserContext *context = (ParserContext *)_context;
	Continuation *lcont = (Continuation *)_cont;

	if(lcont->action->type != ACTION_ACCEPT) {
		return 0;
	}
	Token token = lcont->token;
	Token retry = token;
	Continuation cont;

	int count = 0;
	do {
		cont = fsm_thread_match(&context->thread, &retry);
		listener_notify(&context->parser_transition, &cont);

		// TODO: Temporary continuation, it should be in control loop
	} while (!pda_continuation_follow(&cont, &token, &retry, &count));
	return 0;
}

int user_build_parser(Parser *parser)
{
	listener_init(&parser->parse_setup_lexer, NULL, NULL);
	listener_init(&parser->parse_setup_fsm, NULL, NULL);
	listener_init(&parser->parse_start, ast_parse_start, NULL);
	listener_init(&parser->parse_loop, control_loop_linear, NULL);
	listener_init(&parser->lexer_transition, user_lexer_transition, NULL);
	listener_init(&parser->parser_transition, ast_parser_transition, NULL);
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
