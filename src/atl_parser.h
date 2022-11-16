#ifndef ATL_PARSER_H
#define ATL_PARSER_H

#include "fsm.h"
#include "fsmbuilder.h"
#include "ast.h"
#include "parser.h"


void atl_init_fsm(Fsm *fsm);
int atl_build_parser(Parser *parser);
void atl_ast_transform(Ast *ast, Fsm *source_fsm);

#endif //ATL_PARSER_H
