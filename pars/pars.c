#include "pars.h"
#include "cmemory.h"
#include "input.h"
#include "fsm.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

Ast *pars_load_grammar(char *pathname)
{
	Input *input;
	Ast *ast;
    Fsm *fsm = c_new(Fsm, 1);

	input = input_init(pathname);

	if(input) {
		if(input->is_open) {
			ast = c_new(Ast,1);
			ebnf_input_to_ast(ast, input);

			fsm = c_new(Fsm, 1);
			ebnf_ast_to_fsm(fsm, ast);
		} else {
			printf("Could not open grammar file\n");
		}
	} else {
		printf("Grammar file not found\n");
	}
    return ast;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	if(argc > 1) {
		printf("Loading grammar\n");
		pars_load_grammar(argv[1]);
	} else {
		printf("Usage:\n");
		printf("pars <grammar-file>\n");
	}
    return 0;
}
#endif
