#ifndef EBNF_PARSER_H
#define EBNF_PARSER_H

#include "fsm.h"
#include "fsmbuilder.h"
#include "ast.h"
#include "parser.h"


void ebnf_build_lexer_fsm(Fsm *fsm);
void ebnf_build_fsm(Fsm *fsm);
int ebnf_fsm_ast_handler(int type, void *target, void *args);
int ebnf_build_parser(Parser *parser);
void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast);

#endif //EBNF_PARSER_H
