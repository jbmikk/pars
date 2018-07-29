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
	cont = fsm_thread_loop(&context->thread, token);

	return cont.error;
}

int user_build_parser(Parser *parser)
{
	listener_init(&parser->parse_setup_lexer, NULL, NULL);
	listener_init(&parser->parse_setup_fsm, NULL, NULL);
	listener_init(&parser->parse_start, ast_parse_start, NULL);
	listener_init(&parser->parse_loop, control_loop_linear, NULL);
	listener_init(&parser->lexer_pipe, user_lexer_pipe, NULL);
	listener_init(&parser->parser_pipe, ast_parser_pipe, NULL);
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
