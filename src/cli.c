#include "cli.h"

#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parsercontext.h"
#include "ebnf_parser.h"
#include "astlistener.h"

#include <stddef.h>
#include <stdio.h>

static void identity_init_lexer_fsm(Fsm *fsm)
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
	fsm_thread_match((FsmThread *)thread, token);
}

int _user_setup_lexer(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->lexer_thread.handler.target = &context->thread;
	context->lexer_thread.handler.shift = NULL;
	context->lexer_thread.handler.reduce = NULL;
	context->lexer_thread.handler.accept = _user_pipe_token;
	return 0;
}

int _user_build_parser(Parser *parser)
{
	listener_init(&parser->parse_setup_lexer, _user_setup_lexer, NULL);
	listener_init(&parser->parse_setup_fsm, ast_setup_fsm, NULL);
	listener_init(&parser->parse_start, ast_parse_start, NULL);
	listener_init(&parser->parse_end, ast_parse_end, NULL);
	listener_init(&parser->parse_error, ast_parse_error, NULL);

	//TODO: Maybe basic initialization should be separate from specific
	//initialization for different kinds of parsers.
	//TODO: We don't have a lexer for the source yet
	identity_init_lexer_fsm(&parser->lexer_fsm);
	return 0;
//error:
	//TODO: free

	//return -1;
}

static int _parse_grammar(Parser *ebnf_parser, Input *input, Parser *parser)
{
	Ast ast;
	ParserContext context;
	parser_context_init(&context, ebnf_parser);

	parser_context_set_ast(&context, &ast);
	parser_context_set_input(&context, input);

	int error = parser_context_execute(&context);
	check(!error, "Could not build ebnf ast.");

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(&parser->fsm, &ast);

	//Must be disposed always, unless parser_execute fails?
	ast_dispose(&ast);
	parser_context_dispose(&context);
	return 0;
error:
	parser_context_dispose(&context);
	return -1;
}

int cli_load_grammar(char *pathname, Parser *parser)
{
	Input input;
	Parser ebnf_parser;
	int error;

	parser_init(&ebnf_parser);
	error = ebnf_build_parser(&ebnf_parser);
	check(!error, "Could not build ebnf parser.");

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	error = _parse_grammar(&ebnf_parser, &input, parser);
	check(!error, "Could not parse grammar.");

	parser_dispose(&ebnf_parser);
	input_dispose(&input);

	return 0;
error:
	//TODO: remove risk of disposing uninitialized structures
	// _init functions should not return errors, thus ensuring
	// all of them have been executed and _dispose functions are
	// safe to call.
	parser_dispose(&ebnf_parser);
	input_dispose(&input);

	return -1;
}

#define nzs(S) (S), (strlen(S))

static int _parse_source(Parser *parser, Input *input, Ast *ast)
{
	ParserContext context;
	parser_context_init(&context, parser);

	parser_context_set_ast(&context, ast);
	parser_context_set_input(&context, input);

	int error = parser_context_execute(&context);
	check(!error, "Could not build ast for source.");

	parser_context_dispose(&context);
	return 0;
error:
	parser_context_dispose(&context);
	return -1;
}

int cli_load_source(char *pathname, Parser *parser, Ast *ast)
{
	Input input;
	int error;

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open source file: %s", pathname);

	error = _parse_source(parser, &input, ast);
	check(!error, "Could not parse source: %s", pathname);

	ast_print(ast);

	input_dispose(&input);
	return 0;
error:
	input_dispose(&input);
	return -1;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	Parser parser;
	Ast ast;
	int error;
	if(argc > 1) {
		log_info("Loading grammar.");

		parser_init(&parser);
		error = _user_build_parser(&parser);
		check(!error, "Could not initialize parser.");

		error = cli_load_grammar(argv[1], &parser);
		check(!error, "Could not load grammar.");

		if(argc > 2) {
			log_info("Parsing source.");
			error = cli_load_source(argv[2], &parser, &ast);

			//TODO: no need to dispose on parsing error
			//Safe to dispose twice anyway.
			ast_dispose(&ast);
		}

		parser_dispose(&parser);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file> [<source-file>]");
	}
	return 0;
error:
	parser_dispose(&parser);
	return -1;
}
#endif
