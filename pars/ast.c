#include "ast.h"

#include "cmemory.h"
#include "symbols.h"

#include <stdio.h>

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
	c_radix_tree_set(&node->parent->children, buffer, size, node);
}

void ast_open(Ast *ast, unsigned int index)
{
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, index);

	if(ast->previous != NULL) {
		AstNode *previous = ast->previous;
		ast->previous = NULL;
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

	sibling = (AstNode *)c_radix_tree_get_next(&parent->children, buffer, size);
	if(sibling == NULL && parent->parent != NULL) {
		sibling = ast_get_next_sibling(parent);
	}
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

AstNode *ast_cursor_depth_next(AstCursor *cursor)
{
	AstNode *node;

	if(cursor->current == NULL) {
		node = &cursor->ast->root;
	} else {
		//Get first children
		AstNode *current = cursor->current;
		node = (AstNode *)c_radix_tree_get_next(&current->children, NULL, 0);
		if(node == NULL && current->parent != NULL) {
			node = ast_get_next_sibling(current);
		}
	}
	cursor->current = node;
	return node;
}

AstNode *ast_cursor_depth_next_symbol(AstCursor *cursor, int symbol)
{
	AstNode * node;
	do {
		node = ast_cursor_depth_next(cursor);
	} while(node != NULL && node->symbol != symbol);
	return node;
}

void ast_cursor_dispose(AstCursor *cursor)
{
	cursor->ast = NULL;
	cursor->current = NULL;
	stack_dispose(cursor->stack);
	cursor->stack = NULL;
}
