#include "ebnf_parser.h"

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

void ebnf_parse_expression(LInput *input, E_HANDLER)
{
    unsigned int i;
    switch(NEXT(i)) {
    case L_TERMINAL_STRING:
        handler(E_TERMINAL_STRING);
        break;
    case L_IDENTIFIER:
        handler(E_IDENTIFIER);
        break;
    default:
        ERROR(i);
    }
}

void ebnf_parse_single_definition(LInput *input, E_HANDLER)
{
    unsigned int i;
begin:
    ebnf_parse_expression(input, handler);
    switch(NEXT(i)) {
    case L_CONCATENATE_SYMBOL:
        //parse next expression
        goto begin;
        break;
    case L_EOF:
        ERROR(i);
        break;
    default:
        handler(E_SINGLE_DEFINITION);
        RESTORE(i);
    }
}

void ebnf_parse_definition_list(LInput *input, E_HANDLER)
{
    unsigned int i;
begin:
    ebnf_parse_single_definition(input, handler);
    switch(NEXT(i)) {
    case L_DEFINITION_SEPARATOR_SYMBOL:
        //parse next single rule
        goto begin;
        break;
    case L_EOF:
        ERROR(i);
        break;
    default:
        handler(E_DEFINITION_LIST);
        RESTORE(i);
    }
}

void ebnf_parse_syntax_rule(LInput *input, E_HANDLER)
{
    match(input, L_IDENTIFIER);
    match(input, L_DEFINING_SYMBOL);
    ebnf_parse_definition_list(input, handler);
    match(input, L_TERMINATOR_SYMBOL);
    handler(E_SYNTAX_RULE);
}

void ebnf_parse_syntax(LInput *input, E_HANDLER)
{
    unsigned int i;
begin:
    switch(NEXT(i)) {
    case L_EOF:
        //end
        handler(E_SYNTAX);
        break;
    default:
        RESTORE(i);
        ebnf_parse_syntax_rule(input, handler);
        goto begin;
    }
}

int ebnf_start_parsing(LInput *input, E_HANDLER)
{
    if(setjmp(on_error) != 0) {
        return -1;
    }
    else
    {
        ebnf_parse_syntax(input, handler);
        return 0;
    }
}
