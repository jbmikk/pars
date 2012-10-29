#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "lexer.h"

#define E_HANDLER void (handler)(int token)

typedef enum {
    E_EXPRESSION = -1,
    E_SINGLE_DEFINITION = -2,
    E_DEFINITIONS_LIST = -3,
    E_NON_TERMINAL_DECLARATION = -4,
    E_SYNTAX = -5
} EToken;

int ebnf_start_parsing(LInput *input, E_HANDLER);

#endif //EBNF_PARSER_H
