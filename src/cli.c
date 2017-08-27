#include "cli.h"

#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parsercontext.h"
#include "ebnf_parser.h"
#include "user_parser.h"

#include <stddef.h>
#include <stdio.h>


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
	input_init(&input);

	error = ebnf_build_parser(&ebnf_parser);
	check(!error, "Could not build ebnf parser.");

	error = input_open_file(&input, pathname);
	check(!error, "Could not open grammar file.");

	error = _parse_grammar(&ebnf_parser, &input, parser);
	check(!error, "Could not parse grammar.");

	parser_dispose(&ebnf_parser);
	input_dispose(&input);

	return 0;
error:
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

	input_init(&input);

	error = input_open_file(&input, pathname);
	check(!error, "Could not open input file.");

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
		error = user_build_parser(&parser);
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
