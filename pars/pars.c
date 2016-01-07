#include "pars.h"
#include "dbg.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

Ast *pars_load_grammar(char *pathname)
{
	Input input;
	Ast *ast = NULL;
	Fsm *fsm = c_new(Fsm, 1);
	check_mem(fsm);

	input_init(&input, pathname);

	check(input.is_open, "Could not find or open grammar file: %s", pathname);

	ast = c_new(Ast,1);
	check_mem(ast);
	int error = ebnf_input_to_ast(ast, &input);
	check(!error, "Could not build ebnf ast.");

	fsm = c_new(Fsm, 1);
	check_mem(fsm);
	ebnf_ast_to_fsm(fsm, ast);
	return ast;

error:
	if(input.is_open)
		input_dispose(&input);
	if(ast)
		c_free(ast);
	if(fsm)
		c_free(fsm);

	return NULL;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	if(argc > 1) {
		log_info("Loading grammar.");
		Ast *ast = pars_load_grammar(argv[1]);
		if(ast) {
			ast_dispose(ast);
			c_delete(ast);
		}
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
}
#endif
