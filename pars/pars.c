#include "pars.h"
#include "cmemory.h"
#include "lexer.h"

#include <stddef.h>

void _pars_parse_rule(PGrammar *grammar, LInput *input)
{
    while (!input->eof) {
        lexer_input_next(input);
        //_pars_parse_rule(grammar, input);
    }
}

void _pars_parse_grammar(PGrammar *grammar, LInput *input)
{
    while (!input->eof) {
        _pars_parse_rule(grammar, input);
    }
}

PGrammar *pars_load_grammar(char *pathname)
{
    LInput *input;
    PGrammar *grammar = NULL;

    input = lexer_input_init(pathname);

    if(input->is_open) {
        grammar = c_new(PGrammar,1);
        _pars_parse_grammar(grammar, input);
    }
    return grammar;
}

#ifndef LIBRARY
int main(int argc, char** argv){
    return 0;
}
#endif
