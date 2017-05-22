#include "ast.h"
#include "cmemory.h"

#include <stdio.h>

#ifdef AST_TRACE
#define trace(N, A, T, I, L) printf("ast: %p, %s: %c (%i), at: %i, length: %i \n", N, A, (char)T, T, I, L);
#else
#define trace(N, A, T, I, L)
#endif

void ast_node_init(AstNode *node, AstNode *parent, const Token *token)
{
	node->token = *token;
	radix_tree_init(&node->children);
	node->parent = parent;
}

void ast_init(Ast *ast, Input *input, SymbolTable *table)
{
	ast_node_init(&ast->root, NULL, &(Token){0, 0, 0});
	ast->current = &ast->root;
	ast->previous = NULL;
	ast->input = input;
	ast->table = table;
}

void ast_node_dispose(AstNode *node)
{
	AstNode *an;
	Iterator it;

	radix_tree_iterator_init(&it, &node->children);
	while((an = (AstNode *)radix_tree_iterator_next(&it))) {
		ast_node_dispose(an);
		c_delete(an);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&node->children);
}

void ast_dispose(Ast *ast)
{
	//TODO Write tests for ast disposal
	while(ast->current && ast->current != &ast->root) {
		Token token;
		token_init(&token, 0, 0, 0);
		ast_close(ast, &token);
	}
	ast_done(ast);

	ast_node_dispose(&ast->root);

	//In order to dispose multiple times we need to delete old references
	//Another solution would be to make the root node dynamic.
	radix_tree_init(&ast->root.children);
	ast->table = NULL;
}

void ast_bind_to_parent(AstNode *node)
{
	radix_tree_set_ple_int(&node->parent->children, node->token.index, node);
}

void ast_add(Ast *ast, const Token *token)
{
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, token);

	ast_bind_to_parent(node);

	trace(node, "add", token->symbol, token->index, token->length);
}

void ast_open(void *ast_p, const Token *token)
{
	Ast *ast = (Ast *)ast_p;
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, &(Token){token->index, 0, 0});

	trace(node, "open", '?', token->index, 0);
	AstNode *previous = ast->previous;
	if(previous != NULL) {
		ast->previous = NULL;
		trace(previous, "previous", '?', previous->token.index, 0);
		if(previous->token.index == node->token.index) {
			previous->parent = node;
		}
		ast_bind_to_parent(previous);
	}
	ast->current = node;

	if(previous == NULL || previous->token.index != node->token.index) {
		//Only add shifted symbol if it's not the same
		//that was just closed.
		ast_add(ast, token);
	}
}

void ast_close(void *ast_p, const Token *token)
{
	Ast *ast = (Ast *)ast_p;
	AstNode *node = ast->current;

	node->token.length = token->length;
	node->token.symbol = token->symbol;

	trace(node, "close", token->symbol, token->index, token->length);
	if(ast->previous != NULL) {
		ast_bind_to_parent(ast->previous);
	}
	ast->previous = node;
	ast->current = node->parent;
}

void ast_done(Ast *ast)
{
	if(ast->previous != NULL) {
		trace(ast->previous, "done", 0, ast->previous->index, 0);
		ast_bind_to_parent(ast->previous);
		ast->previous = NULL;
	} else {
		//TODO: warning or sentinel?
	}
}

AstNode *ast_get_next_sibling(AstNode *node) {
	AstNode *parent = node->parent;
	AstNode *sibling;

	sibling = (AstNode *)radix_tree_get_next_ple_int(&parent->children, node->token.index);
	return sibling;
}

void ast_cursor_init(AstCursor *cursor, Ast *ast) 
{
	cursor->ast = ast;
	cursor->current = NULL;
	cursor->stack = NULL;
}

void ast_cursor_push(AstCursor *cursor) 
{
	cursor->stack = stack_push(cursor->stack, cursor->current);
}

void ast_cursor_pop(AstCursor *cursor) 
{
	cursor->current = (AstNode *)cursor->stack->data;
	cursor->stack = stack_pop(cursor->stack);
}

/**
 * 1 - Get first children
 * 2 - If no children then get next sibling
 * 3 - If no next sibling then get parent and go to step 2
 * 4 - If no parent stop
 */
static AstNode *_depth_next(AstCursor *cursor)
{
	AstNode *next;

	if(cursor->current == NULL) {
		next = &cursor->ast->root;
	} else {
		//Get first children
		AstNode *current = cursor->current;
		AstNode *parent = current->parent;
		next = (AstNode *)radix_tree_get_next(&current->children, NULL, 0);
		if(next == NULL && parent != NULL) {
		next_sibling:
			next = ast_get_next_sibling(current);
			if(next == NULL && parent->parent != NULL) {
				current = parent;
				parent = current->parent;
				goto next_sibling;
			}
		}
	}
	return next;
}

AstNode *ast_cursor_depth_next(AstCursor *cursor)
{
	cursor->current = _depth_next(cursor);
	return cursor->current;
}

AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	do {
		node = _depth_next(cursor);
		cursor->current = node;
	} while(node != NULL && node->token.symbol != symbol);
	cursor->current = node;
	return node;
}

AstNode *ast_cursor_next_sibling_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	do {
		node = ast_get_next_sibling(cursor->current);
	} while(node != NULL && node->token.symbol != symbol);
	if(node != NULL) {
		cursor->current = node;
	}
	return node;
}

void ast_cursor_get_string(AstCursor *cursor, char **str, int *length)
{
	*str = (char *)cursor->ast->input->buffer + cursor->current->token.index;
	*length = cursor->current->token.length;
}

void ast_cursor_dispose(AstCursor *cursor)
{
	cursor->ast = NULL;
	cursor->current = NULL;
	stack_dispose(cursor->stack);
	cursor->stack = NULL;
}

void ast_print_node(Ast *ast, AstNode *node, int level) {
	
	AstNode *next = (AstNode *)radix_tree_get_next(&node->children, NULL, 0);
	Symbol *sy;

	if(!next) {
		return;
	}

	do {
		char *src = ast->input->buffer + next->token.index;
		int index = next->token.index;
		int length = next->token.length;

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

	} while((next = ast_get_next_sibling(next)));
}

int ast_get_symbol(AstCursor *cur, char *name, unsigned int length) {
	return symbol_table_get(cur->ast->table, name, length)->id;
}

void ast_print(Ast* ast) {
	ast_print_node(ast, &ast->root, 0);
}
