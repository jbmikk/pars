#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "lexer.h"
#include "fsm.h"
#include "ast.h"
#include "parser.h"

typedef enum {
    E_EXPRESSION = -1,
    E_SINGLE_DEFINITION = -2,
    E_DEFINITIONS_LIST = -3,
    E_NON_TERMINAL_DECLARATION = -4,
    E_SYNTAX = -5
} EToken;

void ebnf_init_fsm(Fsm *fsm);
int ebnf_fsm_ast_handler(int type, void *target, void *args);
int ebnf_init_parser(Parser *parser);
int ebnf_dispose_parser(Parser *parser);
void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast);

#endif //EBNF_PARSER_H
