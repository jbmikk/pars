#ifndef ASTQUERY_H
#define ASTQUERY_H

#include "ast.h"

typedef struct AstQuery {
	AstCursor cursor;
	int *symbols;
	unsigned short length;
} AstQuery;

void ast_query_init(AstQuery *query, Ast *ast, int *symbols, unsigned short length);
void ast_query_dispose(AstQuery *query);
AstNode *ast_query_next(AstQuery *query);

#endif //ASTQUERY_H
