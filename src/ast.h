#ifndef AST_H
#define AST_H

#include "radixtree.h"
#include "stack.h"
#include "input.h"
#include "symbols.h"

typedef struct _AstNode {
	unsigned int index;
	unsigned int length;
	int symbol;
	Node children;
	struct _AstNode *parent;
} AstNode;

typedef struct _Ast {
	AstNode root;
	AstNode *current;
	AstNode *previous;
	SymbolTable *table;
	Input *input;
} Ast;

typedef struct _AstCursor {
	Ast *ast;
	AstNode *current;
	SNode *stack;
} AstCursor;

void ast_init(Ast *ast, Input *input, SymbolTable *table);
void ast_dispose(Ast *ast);
void ast_open(void *ast_p, unsigned int index, unsigned int length, int symbol);
void ast_close(void *ast_p, unsigned int index, unsigned int length, int symbol);
void ast_done(Ast *ast);
void ast_print(Ast *ast);
int ast_get_symbol(AstCursor *cur, char *name, unsigned int length);
void ast_cursor_init(AstCursor *cursor, Ast *ast);
void ast_cursor_push(AstCursor *cursor);
void ast_cursor_pop(AstCursor *cursor);
AstNode *ast_cursor_depth_next(AstCursor *cursor);
AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol);
AstNode *ast_cursor_next_sibling_symbol(AstCursor *cursor, int symbol);
void ast_cursor_get_string(AstCursor *cursor, unsigned char **str, int *length);
void ast_cursor_dispose(AstCursor *cursor);

#endif //AST_H
