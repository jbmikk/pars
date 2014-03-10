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

typedef struct _AstCursor {
	Ast *ast;
	AstNode *current;
} AstCursor;

void ast_init(Ast *ast);
void ast_dispose(Ast *ast);
void ast_open(Ast *ast, unsigned int index);
void ast_close(Ast *ast, unsigned int index, unsigned int length, int symbol);
void ast_done(Ast *ast);
void ast_cursor_init(AstCursor *cursor, Ast *ast);
AstNode *ast_cursor_depth_next(AstCursor *cursor);
AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol);
void ast_cursor_dispose(AstCursor *cursor);

#endif //AST_H
