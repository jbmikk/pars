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
	Input *input;
	Ast *ast = NULL;
	Fsm *fsm = c_new(Fsm, 1);
	check_mem(fsm);

	input = input_init(pathname);

	check(input, "Grammar file not found: %s", pathname);
	check(input->is_open, "Could not open grammar file: %s", pathname);

	ast = c_new(Ast,1);
	check_mem(ast);
	ebnf_input_to_ast(ast, input);

	fsm = c_new(Fsm, 1);
	check_mem(fsm);
	ebnf_ast_to_fsm(fsm, ast);
	return ast;

error:
	if(input && input->is_open)
		input_close(input);
	if(ast)
		c_free(ast);
	if(fsm)
		c_free(ast);

	return NULL;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	if(argc > 1) {
		log_info("Loading grammar.");
		pars_load_grammar(argv[1]);
	} else {
		log_info("Usage:");
		log_info("pars <grammar-file>");
	}
	return 0;
}
#endif
