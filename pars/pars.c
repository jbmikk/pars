#include "pars.h"
#include "cmemory.h"
#include "lexer.h"

#include <stddef.h>
#include <stdio.h>

void _pars_parse_grammar(PGrammar *grammar, LInput *input)
{
	Fsm ebnf;
	init_ebnf_parser(&ebnf);
    Session *session = fsm_start_session(&ebnf);
    while (!input->eof) {
		LToken token = lexer_input_next(input);
		session_match(session, token, lexer_input_get_index(input));
    }
}

PGrammar *pars_load_grammar(char *pathname)
{
    LInput *input;
    PGrammar *grammar = NULL;

    input = lexer_input_init(pathname);

	if(input) {
		if(input->is_open) {
			grammar = c_new(PGrammar,1);
			_pars_parse_grammar(grammar, input);
		} else {
			printf("Could not open grammar file\n");
		}
	} else {
		printf("Grammar file not found\n");
	}
    return grammar;
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
