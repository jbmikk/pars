#include "cli.h"

#include "dbg.h"
#include "cmemory.h"
#include "source.h"
#include "fsm.h"
#include "parsercontext.h"
#include "ebnf_parser.h"
#include "user_parser.h"

#include <stddef.h>
#include <stdio.h>

static int _parse_source(Parser *parser, Source *source, Ast *ast)
{
	ParserContext context;
	parser_context_init(&context, parser);

	parser_context_set_ast(&context, ast);
	parser_context_set_source(&context, source);

	int error = parser_context_execute(&context);
	check(!error, "Could not build ast for source.");

	parser_context_dispose(&context);
	return 0;
error:
	parser_context_dispose(&context);
	return -1;
}

static int _parse_grammar(Parser *ebnf_parser, Source *source, Parser *parser)
{
	Ast ast;

	int error = _parse_source(ebnf_parser, source, &ast);
	check(!error, "Could not build ebnf ast.");

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(&parser->fsm, &ast);

	ast_dispose(&ast);
	return 0;
error:
	//TODO: verify what happens parser_execute fails? should probably dispose AST
	return -1;
}

int cli_load_grammar(Params *params, Parser *parser)
{
	Source source;
	Parser ebnf_parser;
	int error;

	parser_init(&ebnf_parser);
	source_init(&source);

	error = ebnf_build_parser(&ebnf_parser);
	check(!error, "Could not build ebnf parser.");

	error = source_open_file(&source, params->param);
	check(!error, "Could not open grammar file.");

	error = _parse_grammar(&ebnf_parser, &source, parser);
	check(!error, "Could not parse grammar.");

	parser_dispose(&ebnf_parser);
	source_dispose(&source);

	return 0;
error:
	parser_dispose(&ebnf_parser);
	source_dispose(&source);

	return -1;
}

#define nzs(S) (S), (strlen(S))

int cli_load_target(Params *params, Parser *parser, Ast *ast)
{
	Source source;
	int error;

	source_init(&source);

	error = source_open_file(&source, params->param);
	check(!error, "Could not open source file.");

	error = _parse_source(parser, &source, ast);
	check(!error, "Could not parse source: %s", params->param);

	ast_print(ast);

	source_dispose(&source);
	return 0;
error:
	source_dispose(&source);
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

		Params params1 = { argv[1] };
		error = cli_load_grammar(&params1, &parser);
		check(!error, "Could not load grammar.");

		if(argc > 2) {
			log_info("Parsing target.");
			Params params2 = { argv[2] };
			error = cli_load_target(&params2, &parser, &ast);

			//TODO: no need to dispose on parsing error
			//Safe to dispose twice anyway.
			ast_dispose(&ast);
		}

		parser_dispose(&parser);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file> [<target-file>]");
	}
	return 0;
error:
	parser_dispose(&parser);
	return -1;
}
#endif
