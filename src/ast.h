#ifndef AST_H
#define AST_H

#include "radixtree.h"
#include "stack.h"
#include "input.h"
#include "symbols.h"
#include "token.h"

typedef struct AstNode {
	Token token;
	Node children;
	struct AstNode *parent;
} AstNode;

typedef struct Ast {
	AstNode root;
	SymbolTable *table;
	Input *input;
} Ast;

typedef struct AstCursor {
	Ast *ast;
	AstNode *current;
	int offset;
	Stack stack;
} AstCursor;

void ast_node_init(AstNode *node, AstNode *parent, const Token *token);
void ast_node_dispose(AstNode *node);

void ast_init(Ast *ast, Input *input, SymbolTable *table);
void ast_dispose(Ast *ast);
void ast_print(Ast *ast);
int ast_get_symbol(AstCursor *cur, char *name, unsigned int length);
void ast_cursor_init(AstCursor *cursor, Ast *ast);
void ast_cursor_push(AstCursor *cursor);
void ast_cursor_pop(AstCursor *cursor);
AstNode *ast_cursor_depth_next(AstCursor *cursor);
AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol);
AstNode *ast_cursor_descendant_next_symbol(AstCursor *cursor, int symbol);
AstNode *ast_cursor_relative_next_symbol(AstCursor *cursor, int symbol);
AstNode *ast_cursor_desc_or_rel_next_symbol(AstCursor *cursor, int symbol);
AstNode *ast_cursor_next_sibling_symbol(AstCursor *cursor, int symbol);
void ast_cursor_get_string(AstCursor *cursor, char **str, int *length);
void ast_cursor_dispose(AstCursor *cursor);

#endif //AST_H
