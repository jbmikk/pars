#include <setjmp.h>

#include "lexer.h"

jmp_buf on_error;

void parse_error(LInput *input, unsigned int index)
{
    longjmp(on_error, 1);
}

void match(LInput *input, LToken token)
{
    LToken t;
    t = lexer_input_next(input);
    if(t != token) {
        parse_error(input, lexer_input_get_index(input));
    }
}

#define NEXT(V) (\
        V = lexer_input_get_index(input),\
        lexer_input_next(input)\
    )

#define RESTORE(V) lexer_input_set_index(input, V);

#define ERROR(V) parse_error(input, V)

void ebnf_parse_expression(LInput *input)
{
    unsigned int i;
    switch(NEXT(i)) {
    case L_TERMINAL_STRING:
        //add terminal
        break;
    case L_IDENTIFIER:
        //add non terminal
        break;
    default:
        ERROR(i);
    }
}

void ebnf_parse_single_rule(LInput *input)
{
    unsigned int i;
begin:
    ebnf_parse_expression(input);
    switch(NEXT(i)) {
    case L_CONCATENATE_SYMBOL:
        //parse next expression
        goto begin;
        break;
    case L_EOF:
        ERROR(i);
        break;
    default:
        RESTORE(i);
    }
}

void ebnf_parse_rule_body(LInput *input)
{
    unsigned int i;
begin:
    ebnf_parse_single_rule(input);
    switch(NEXT(i)) {
    case L_DEFINITION_SEPARATOR_SYMBOL:
        //parse next single rule
        goto begin;
        break;
    case L_EOF:
        ERROR(i);
        break;
    default:
        RESTORE(i);
    }
}

void ebnf_parse_rule(LInput *input)
{
    match(input, L_IDENTIFIER);
    match(input, L_DEFINING_SYMBOL);
    ebnf_parse_rule_body(input);
    match(input, L_TERMINATOR_SYMBOL);
}

void ebnf_parse_grammar(LInput *input)
{
    unsigned int i;
begin:
    switch(NEXT(i)) {
    case L_EOF:
        //end
        break;
    default:
        RESTORE(i);
        ebnf_parse_rule(input);
        goto begin;
    }
}

int ebnf_start_parsing(LInput *input)
{
    if(setjmp(on_error) != 0) {
        return -1;
    }
    else
    {
        ebnf_parse_grammar(input);
        return 0;
    }
}
