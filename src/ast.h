#ifndef AST_H
#define AST_H

#include "rtree.h"
#include "stack.h"
#include "source.h"
#include "symbols.h"
#include "token.h"

typedef struct AstNode AstNode;

DEFINE(BMap, unsigned int, AstNode *, AstNode, astnode)

struct AstNode {
	Token token;
	BMapAstNode children;
	struct AstNode *parent;
};

typedef struct Ast {
	AstNode root;
	SymbolTable *table;
	Source *source;
} Ast;

DEFINE(Stack, AstNode *, AstNode, astnode);

typedef struct AstCursor {
	Ast *ast;
	AstNode *current;
	int offset;
	StackAstNode stack;
} AstCursor;


PROTOTYPES(BMap, unsigned int, AstNode *, AstNode, astnode)

void ast_node_init(AstNode *node, AstNode *parent, const Token *token);
void ast_node_dispose(AstNode *node);

void ast_init(Ast *ast, Source *source, SymbolTable *table);
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
