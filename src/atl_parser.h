#ifndef ATL_PARSER_H
#define ATL_PARSER_H

#include "fsm.h"
#include "fsmbuilder.h"
#include "ast.h"
#include "parser.h"


void atl_init_fsm(Fsm *fsm);
int atl_init_parser(Parser *parser);
int atl_dispose_parser(Parser *parser);
void atl_ast_transform(Ast *ast);

#endif //ATL_PARSER_H
