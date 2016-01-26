#include "pars.h"
#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parser.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

int pars_load_grammar(char *pathname, Fsm *fsm)
{
	Input input;
	Parser parser;
	Ast ast;
	int error;

	fsm_init(fsm);

	error = ebnf_init_parser(&parser);
	check(!error, "Could not build ebnf parser.");

	input_init(&input, pathname);
	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	error = parser_execute(&parser, &ast, &input);
	check(!error, "Could not build ebnf ast.");

	ebnf_dispose_parser(&parser);

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(fsm, &ast);

	ast_dispose(&ast);
	input_dispose(&input);

	return 0;
error:
	//TODO: remove risk of disposing uninitialized structures
	// _init functions should not return errors, thus ensuring
	// all of them have been executed and _dispose functions are
	// safe to call.
	ebnf_dispose_parser(&parser);

	if(input.is_open)
		input_dispose(&input);

	fsm_dispose(fsm);

	return -1;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	Fsm fsm;
	int error;
	if(argc > 1) {
		log_info("Loading grammar.");
		error = pars_load_grammar(argv[1], &fsm);
		check(!error, "Could not load grammar.");

		fsm_dispose(&fsm);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
error:
	return -1;
}
#endif
