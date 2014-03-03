#ifndef AST_H
#define AST_H

#include "cradixtree.h"

typedef struct _AstNode {
	unsigned int index;
	unsigned int length;
	int symbol;
	CNode children;
	struct _AstNode *parent;
} AstNode;

typedef struct _Ast {
	AstNode root;
	AstNode *current;
	AstNode *previous;
} Ast;

void ast_init(Ast *ast);
void ast_open(Ast *ast, unsigned int index);
void ast_close(Ast *ast, unsigned int index, unsigned int length, int symbol);
void ast_push(Ast *ast);

#endif //AST_H
