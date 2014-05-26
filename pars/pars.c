#include "pars.h"
#include "cmemory.h"
#include "lexer.h"
#include "fsm.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

void pars_ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor cursor;
	AstNode *node;

	ast_cursor_init(&cursor, ast);
	node = ast_cursor_depth_next(&cursor);
	ast_cursor_dispose(&cursor);
}

void pars_ebnf_input_to_ast(Ast *ast, LInput *input)
{
    Fsm *ebnf_fsm = c_new(Fsm, 1);

	EventListener ebnf_listener;
	ebnf_listener.target = ast;
	ebnf_listener.handler = ebnf_fsm_ast_handler;

	ebnf_init_fsm(ebnf_fsm);
	ast_init(ast);

    Session *session = fsm_start_session(ebnf_fsm);
	session_set_listener(session, ebnf_listener);

    while (!input->eof) {
		LToken token = lexer_input_next(input);
		session_match(session, token, lexer_input_get_index(input));
    }
}

Ast *pars_load_grammar(char *pathname)
{
    LInput *input;
	Ast *ast;
    Fsm *fsm = c_new(Fsm, 1);

    input = lexer_input_init(pathname);

	if(input) {
		if(input->is_open) {
			ast = c_new(Ast,1);
			pars_ebnf_input_to_ast(ast, input);

			fsm = c_new(Fsm, 1);
			pars_ebnf_ast_to_fsm(fsm, ast);
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
	}
    return 0;
}
#endif
