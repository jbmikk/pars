#include "ast.h"
#include "cmemory.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef AST_TRACE
#define trace(N, A, T, I, L) printf("ast: %p, %s: %c (%i), at: %i, length: %i \n", N, A, (char)T, T, I, L);
#else
#define trace(N, A, T, I, L)
#endif

FUNCTIONS(Stack, AstNode *, AstNode, astnode);

FUNCTIONS(BMap, unsigned int, AstNode *, AstNode, astnode)

void ast_node_init(AstNode *node, AstNode *parent, const Token *token)
{
	node->token = *token;
	bmap_astnode_init(&node->children);
	node->parent = parent;
}

void ast_node_dispose(AstNode *node)
{
	AstNode *an;
	BMapCursorAstNode cursor;

	bmap_cursor_astnode_init(&cursor, &node->children);
	while(bmap_cursor_astnode_next(&cursor)) {
		an = bmap_cursor_astnode_current(&cursor)->astnode;
		ast_node_dispose(an);
		free(an);
	}
	bmap_cursor_astnode_dispose(&cursor);

	bmap_astnode_dispose(&node->children);
}


void ast_init(Ast *ast, Source *source, SymbolTable *table)
{
	ast_node_init(&ast->root, NULL, &(Token){0, 0, 0});
	ast->source = source;
	ast->table = table;
}

void ast_dispose(Ast *ast)
{
	ast_node_dispose(&ast->root);

	//In order to dispose multiple times we need to delete old references
	//Another solution would be to make the root node dynamic.
	bmap_astnode_init(&ast->root.children);
	ast->table = NULL;
}

void ast_cursor_init(AstCursor *cursor, Ast *ast) 
{
	cursor->ast = ast;
	cursor->current = NULL;
	stack_astnode_init(&cursor->stack);
}

void ast_cursor_push(AstCursor *cursor) 
{
	//TODO: validate errors?
	stack_astnode_push(&cursor->stack, cursor->current);
}

void ast_cursor_pop(AstCursor *cursor) 
{
	cursor->current = stack_astnode_top(&cursor->stack);
	stack_astnode_pop(&cursor->stack);
}

static AstNode *_get_next_sibling(AstNode *node) {
	AstNode *parent = node->parent;
	BMapEntryAstNode *entry;

	entry = bmap_astnode_get_gt(&parent->children, node->token.index);

	return entry? entry->astnode: NULL;
}

static AstNode *_depth_out_next(AstCursor *cursor, AstNode *base)
{
	AstNode *current = cursor->current;
	AstNode *parent = current->parent;
	AstNode *next;
next_sibling:
	if(parent) {
		next = _get_next_sibling(current);
		if(next == NULL) {
			cursor->offset--;
			if(parent != base) {
				current = parent;
				parent = current->parent;
				goto next_sibling;
			}
		}
	} else {
		next = NULL;
	}
	return next;
}
/**
 * 1 - Get first children
 * 2 - If no children then get next sibling
 * 3 - If no next sibling then get parent and go to step 2
 * 4 - If no parent stop
 */
static AstNode *_depth_next(AstCursor *cursor, AstNode *base)
{
	AstNode *next;

	if(cursor->current == NULL) {
		if(base) {
			next = base;
		} else {
			next = base = &cursor->ast->root;
		}
	} else {
		//Get first children
		AstNode *current = cursor->current;
		BMapEntryAstNode *entry = bmap_astnode_first(&current->children);
		if(entry == NULL) {
			next = _depth_out_next(cursor, base);
		} else {
			next = entry->astnode;
			cursor->offset++;
		}
	}
	return next;
}

AstNode *ast_cursor_depth_next(AstCursor *cursor)
{
	cursor->offset = 0;
	cursor->current = _depth_next(cursor, &cursor->ast->root);
	return cursor->current;
}

AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	cursor->offset = 0;
	do {
		node = _depth_next(cursor, &cursor->ast->root);
		cursor->current = node;
	} while(node != NULL && node->token.symbol != symbol);
	cursor->current = node;
	return node;
}

AstNode *ast_cursor_descendant_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode *node;
	AstNode *base = cursor->current;
	do {
		node = _depth_next(cursor, base);
		cursor->current = node;
	} while(node != NULL && node->token.symbol != symbol);
	cursor->current = node;
	return node;
}

AstNode *ast_cursor_relative_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode *node;
	AstNode *base = stack_astnode_top(&cursor->stack);

	node = _depth_out_next(cursor, base);
	cursor->current = node;
	while(node != NULL && node->token.symbol != symbol) {
		node = _depth_next(cursor, base);
		cursor->current = node;
	}
	cursor->current = node;
	return node;
}

AstNode *ast_cursor_desc_or_rel_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode *node;
	AstNode *base = stack_astnode_top(&cursor->stack);

	do {
		node = _depth_next(cursor, base);
		cursor->current = node;
	} while(node != NULL && node->token.symbol != symbol);
	cursor->current = node;
	return node;
}


AstNode *ast_cursor_next_sibling_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node = cursor->current;
	do {
		node = _get_next_sibling(node);
	} while(node != NULL && node->token.symbol != symbol);
	if(node != NULL) {
		cursor->current = node;
	}
	return node;
}

void ast_cursor_get_string(AstCursor *cursor, char **str, int *length)
{
	*str = (char *)cursor->ast->source->buffer + cursor->current->token.index;
	*length = cursor->current->token.length;
}

void ast_cursor_dispose(AstCursor *cursor)
{
	cursor->ast = NULL;
	cursor->current = NULL;
	stack_astnode_dispose(&cursor->stack);
}

void ast_print_node(Ast *ast, AstNode *node, int level) {
	
	BMapEntryAstNode *entry = bmap_astnode_first(&node->children);
	Symbol *sy;

	if(!entry) {
		return;
	}

	AstNode *next = entry->astnode;

	do {
		char *src = NULL;
		int index = next->token.index;
		int length = 0;

		if(ast->source) {
			src = ast->source->buffer + next->token.index;
			length = next->token.length;
		}

		unsigned char levelstr[level+1];
		int i;
		for(i = 0; i < level; i++) {
			levelstr[i] = '-';
		}
		levelstr[level] = '\0';

		sy = symbol_table_get_by_id(ast->table, next->token.symbol);
		if(sy) {
			printf(
				"(%-3i, %-3i)%s> [%s] %.*s\n",
				index,
				length,
				levelstr,
				sy->name,
				length,
				src
			);
		} else {
			printf(
				"(%-3i, %-3i)%s> T %.*s\n",
				index,
				length,
				levelstr,
				length,
				src
			);
		}

		ast_print_node(ast, next, level+1);

	} while((next = _get_next_sibling(next)));
}

int ast_get_symbol(AstCursor *cur, char *name, unsigned int length) {
	return symbol_table_get(cur->ast->table, name, length)->id;
}

void ast_print(Ast* ast) {
	ast_print_node(ast, &ast->root, 0);
}
