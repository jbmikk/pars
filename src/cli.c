#include "cli.h"

#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parser.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

int cli_load_grammar(char *pathname, Fsm *fsm)
{
	Input input;
	Parser parser;
	Ast ast;
	int error;

	error = ebnf_init_parser(&parser);
	check(!error, "Could not build ebnf parser.");

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	error = parser_execute(&parser, &ast, &input);
	check(!error, "Could not build ebnf ast.");

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(fsm, &ast);

	//Must be disposed always, unless parser_execute fails.
	ast_dispose(&ast);

	//TODO?: can't dispose parser before ast, shared symbol table
	ebnf_dispose_parser(&parser);
	input_dispose(&input);

	return 0;
error:
	//TODO: remove risk of disposing uninitialized structures
	// _init functions should not return errors, thus ensuring
	// all of them have been executed and _dispose functions are
	// safe to call.
	ebnf_dispose_parser(&parser);
	input_dispose(&input);

	return -1;
}

int cli_parse_source(char *pathname, Fsm *fsm, Ast *ast)
{
	Input input;
	Parser parser;
	int error;

	//TODO: Maybe the fsm should be a pointer.
	parser.fsm = *fsm;
	parser.handler.shift = ast_open;
	parser.handler.reduce = ast_close;
	parser.lexer_fsm = identity_lexer;
	//TODO: Please kill me
	parser.table = *fsm->table;

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open source file: %s", pathname);

	error = parser_execute(&parser, ast, &input);
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
	SymbolTable table;
	Fsm fsm;
	Ast ast;
	int error;
	if(argc > 1) {
		log_info("Loading grammar.");

		symbol_table_init(&table);
		fsm_init(&fsm, &table);

		error = cli_load_grammar(argv[1], &fsm);
		check(!error, "Could not load grammar.");

		if(argc > 2) {
			log_info("Parsing source.");
			error = cli_parse_source(argv[2], &fsm, &ast);

			//TODO: no need to dispose on parsing error
			//Safe to dispose twice anyway.
			ast_dispose(&ast);
		}

		fsm_dispose(&fsm);
		symbol_table_dispose(&table);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
error:
	fsm_dispose(&fsm);
	symbol_table_dispose(&table);
	return -1;
}
#endif
