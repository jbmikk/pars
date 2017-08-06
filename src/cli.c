#include "cli.h"

#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parser.h"
#include "ebnf_parser.h"

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

int _user_parser_init(Parser *parser)
{
	parser->handler.shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->handler.accept = NULL;

	parser->lexer_handler.shift = NULL;
	parser->lexer_handler.reduce = NULL;
	parser->lexer_handler.accept = NULL;

	symbol_table_init(&parser->table);

	fsm_init(&parser->lexer_fsm, &parser->table);
	fsm_init(&parser->fsm, &parser->table);

	//TODO: Maybe basic initialization should be separate from specific
	//initialization for different kinds of parsers.
	//TODO: We don't have a lexer for the source yet
	identity_init_lexer_fsm(&parser->lexer_fsm);
	return 0;
//error:
	//TODO: free

	//return -1;
}

void _user_parser_dispose(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	fsm_dispose(&parser->lexer_fsm);
	symbol_table_dispose(&parser->table);
}

int cli_load_grammar(char *pathname, Parser *parser)
{
	Input input;
	Parser ebnf_parser;
	Ast ast;
	int error;

	error = ebnf_init_parser(&ebnf_parser);
	check(!error, "Could not build ebnf parser.");

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	error = parser_execute(&ebnf_parser, &ast, &input);
	check(!error, "Could not build ebnf ast.");

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(&parser->fsm, &ast);

	//Must be disposed always, unless parser_execute fails.
	ast_dispose(&ast);

	//TODO?: can't dispose parser before ast, shared symbol table
	ebnf_dispose_parser(&ebnf_parser);
	input_dispose(&input);

	return 0;
error:
	//TODO: remove risk of disposing uninitialized structures
	// _init functions should not return errors, thus ensuring
	// all of them have been executed and _dispose functions are
	// safe to call.
	ebnf_dispose_parser(&ebnf_parser);
	input_dispose(&input);

	return -1;
}

#define nzs(S) (S), (strlen(S))

int cli_parse_source(char *pathname, Parser *parser, Ast *ast)
{
	Input input;
	int error;

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open source file: %s", pathname);

	error = parser_execute(parser, ast, &input);
	check(!error, "Could not build ast for source %s.", pathname);

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

		error = _user_parser_init(&parser);
		check(!error, "Could not initialize parser.");

		error = cli_load_grammar(argv[1], &parser);
		check(!error, "Could not load grammar.");

		if(argc > 2) {
			log_info("Parsing source.");
			error = cli_parse_source(argv[2], &parser, &ast);

			//TODO: no need to dispose on parsing error
			//Safe to dispose twice anyway.
			ast_dispose(&ast);
		}

		_user_parser_dispose(&parser);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
error:
	_user_parser_dispose(&parser);
	return -1;
}
#endif
