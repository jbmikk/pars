#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "fsm.h"
#include "lexer.h"
#include "processor.h"

typedef enum {
    E_EXPRESSION = -1,
    E_SINGLE_DEFINITION = -2,
    E_DEFINITIONS_LIST = -3,
    E_NON_TERMINAL_DECLARATION = -4,
    E_SYNTAX = -5
} EToken;

void init_ebnf_fsm(Fsm *fsm);
void init_ebnf_interpreter(Processor *processor, Fsm *fsm);

#endif //EBNF_PARSER_H
