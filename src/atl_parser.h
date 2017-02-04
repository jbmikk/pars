#ifndef ATL_PARSER_H
#define ATL_PARSER_H

#include "lexer.h"
#include "atl_lexer.h"
#include "fsm.h"
#include "fsmcursor.h"
#include "ast.h"
#include "parser.h"


void atl_init_fsm(Fsm *fsm);
int atl_init_parser(Parser *parser);
int atl_dispose_parser(Parser *parser);
void atl_ast_transform(Ast *ast);

#endif //ATL_PARSER_H
