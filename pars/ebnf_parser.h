#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "fsm.h"
#include "lexer.h"

typedef enum {
    E_EXPRESSION = -1,
    E_SINGLE_DEFINITION = -2,
    E_DEFINITIONS_LIST = -3,
    E_NON_TERMINAL_DECLARATION = -4,
    E_SYNTAX = -5
} EToken;

void ebnf_init_fsm(Fsm *fsm);

#endif //EBNF_PARSER_H
