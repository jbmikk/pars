#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "lexer.h"

#define E_HANDLER void (handler)(int token)

typedef enum {
    E_TERMINAL_STRING,
    E_IDENTIFIER,
    E_SINGLE_DEFINITION,
    E_DEFINITION_LIST,
    E_SYNTAX_RULE,
    E_SYNTAX
} EToken;

int ebnf_start_parsing(LInput *input, E_HANDLER);

#endif //EBNF_PARSER_H
