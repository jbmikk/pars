#include "ast.h"

#include "cmemory.h"
#include "symbols.h"

#include <stdio.h>

#ifdef AST_TRACE
#define trace(N, A, T, I, L) printf("ast: %i, %s: %c, at: %i, length: %i \n", N, A, (char)T, I, L);
#else
#define trace(N, A, T, I, L)
#endif

void ast_node_init(AstNode *node, AstNode *parent, unsigned int index)
{
	NODE_INIT(node->children, 0, 0, NULL);
	node->parent = parent;
	node->index = index;
}

void ast_init(Ast *ast, Input *input)
{
	ast_node_init(&ast->root, NULL, 0);
	ast->current = &ast->root;
	ast->previous = NULL;
	ast->input = input;
}

void ast_dispose(Ast *ast)
{
    //TODO
}

void ast_bind_to_parent(AstNode *node)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, node->index);
	radix_tree_set(&node->parent->children, buffer, size, node);
}

void ast_open(Ast *ast, unsigned int index)
{
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, index);

	trace(node, "open", '?', index, 0);
	if(ast->previous != NULL) {
		AstNode *previous = ast->previous;
		ast->previous = NULL;
		trace(previous, "previous", '?', previous->index, 0);
		if(previous->index == node->index)
			previous->parent = node;
		ast_bind_to_parent(previous);
	}
	ast->current = node;
}

void ast_close(Ast *ast, unsigned int index, unsigned int length, int symbol)
{
	AstNode *node = ast->current;

	node->length = length;
	node->symbol = symbol;

	trace(node, "close", symbol, index, length);
	if(ast->previous != NULL) {
		ast_bind_to_parent(ast->previous);
	}
	ast->previous = node;
	ast->current = node->parent;
}

void ast_done(Ast *ast)
{
	if(ast->previous != NULL) {
		ast_bind_to_parent(ast->previous);
		ast->previous = NULL;
	}
}

AstNode *ast_get_next_sibling(AstNode *node) {
	AstNode *parent = node->parent;
	AstNode *sibling;

	unsigned char buffer[sizeof(int)];
	unsigned int size;
	symbol_to_buffer(buffer, &size, node->index);

	sibling = (AstNode *)radix_tree_get_next(&parent->children, buffer, size);
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
AstNode *ast_cursor_depth_next(AstCursor *cursor)
{
	AstNode *next;

	if(cursor->current == NULL) {
		next = cursor->current = &cursor->ast->root;
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
			} else {
				cursor->current = next;
			}
		} else {
			cursor->current = next;
		}
	}
	return next;
}

AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	do {
		node = ast_cursor_depth_next(cursor);
	} while(node != NULL && node->symbol != symbol);
	return node;
}

AstNode *ast_cursor_next_sibling_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	do {
		node = ast_get_next_sibling(cursor->current);
	} while(node != NULL && node->symbol != symbol);
	if(node != NULL) {
		cursor->current = node;
	}
	return node;
}

void ast_cursor_get_string(AstCursor *cursor, unsigned char **str, int *length)
{
	*str = cursor->ast->input->buffer + cursor->current->index;
	*length = cursor->current->length;
}

void ast_cursor_dispose(AstCursor *cursor)
{
	cursor->ast = NULL;
	cursor->current = NULL;
	stack_dispose(cursor->stack);
	cursor->stack = NULL;
}
