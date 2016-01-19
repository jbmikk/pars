#include "pars.h"
#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "parser.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

Fsm *pars_load_grammar(char *pathname)
{
	Input input;
	Parser parser;
	Ast ast;
	Fsm *fsm = NULL;
	int error;

	input_init(&input, pathname);

	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	error = ebnf_init_parser(&parser);
	check(!error, "Could not build ebnf parser.");

	error = parser_execute(&parser, &ast, &input);
	check(!error, "Could not build ebnf ast.");

	ebnf_dispose_parser(&parser);

	fsm = c_new(Fsm, 1);
	check_mem(fsm);

	fsm_init(fsm);

	//TODO: make optional under a -v flag
	ast_print(&ast);

	ebnf_ast_to_fsm(fsm, &ast);

	ast_dispose(&ast);
	input_dispose(&input);

	return fsm;

error:
	if(input.is_open)
		input_dispose(&input);

	if(fsm) {
		fsm_dispose(fsm);
		c_delete(fsm);
	}

	return NULL;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	if(argc > 1) {
		log_info("Loading grammar.");
		Fsm *fsm = pars_load_grammar(argv[1]);
		check(fsm, "Could not load grammar.");
		if(fsm) {
			fsm_dispose(fsm);
			c_delete(fsm);
		}
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
error:
	return -1;
}
#endif
