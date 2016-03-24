#include "ast.h"

#include "cmemory.h"
#include "radixtree_ext.h"

#include <stdio.h>

#ifdef AST_TRACE
#define trace(N, A, T, I, L) printf("ast: %p, %s: %c (%i), at: %i, length: %i \n", N, A, (char)T, T, I, L);
#else
#define trace(N, A, T, I, L)
#endif

void ast_node_init(AstNode *node, AstNode *parent, unsigned int index)
{
	radix_tree_init(&node->children, 0, 0, NULL);
	node->parent = parent;
	node->index = index;
	//TODO: Should initialize other fields even if not always used?
}

void ast_init(Ast *ast, Input *input, SymbolTable *table)
{
	ast_node_init(&ast->root, NULL, 0);
	ast->root.symbol = 0;
	ast->current = &ast->root;
	ast->previous = NULL;
	ast->input = input;
	ast->table = table;
}

void _ast_dispose_node(AstNode *node)
{
	AstNode *an;
	Iterator it;

	radix_tree_iterator_init(&it, &node->children);
	while(an = (AstNode *)radix_tree_iterator_next(&it)) {
		_ast_dispose_node(an);
		c_delete(an);
	}
	radix_tree_iterator_dispose(&it);

	radix_tree_dispose(&node->children);
}

void ast_dispose(Ast *ast)
{
	_ast_dispose_node(&ast->root);

	if(ast->previous) {
		//TODO: write test for open ast tree
		_ast_dispose_node(ast->previous);
	}
	ast->table = NULL;
}

void ast_bind_to_parent(AstNode *node)
{
	radix_tree_set_int(&node->parent->children, node->index, node);
}

void ast_add(Ast *ast, unsigned int index, unsigned int length, int symbol)
{
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, index);

	node->length = length;
	node->symbol = symbol;
	ast_bind_to_parent(node);

	trace(node, "add", symbol, index, length);
}

void ast_open(void *ast_p, unsigned int index, unsigned int length, int symbol)
{
	Ast *ast = (Ast *)ast_p;
	AstNode *node = c_new(AstNode, 1);
	ast_node_init(node, ast->current, index);

	trace(node, "open", '?', index, 0);
	AstNode *previous = ast->previous;
	if(previous != NULL) {
		ast->previous = NULL;
		trace(previous, "previous", '?', previous->index, 0);
		if(previous->index == node->index) {
			previous->parent = node;
		}
		ast_bind_to_parent(previous);
	}
	ast->current = node;

	if(previous == NULL || previous->index != node->index) {
		//Only add shifted symbol if it's not the same
		//that was just closed.
		ast_add(ast, index, length, symbol);
	}
}

void ast_close(void *ast_p, unsigned int index, unsigned int length, int symbol)
{
	Ast *ast = (Ast *)ast_p;
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
	trace(ast->previous, "done", 0, ast->previous->index, 0);
	if(ast->previous != NULL) {
		ast_bind_to_parent(ast->previous);
		ast->previous = NULL;
	}
}

AstNode *ast_get_next_sibling(AstNode *node) {
	AstNode *parent = node->parent;
	AstNode *sibling;

	sibling = (AstNode *)radix_tree_get_next_int(&parent->children, node->index);
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

void ast_print_node(Ast *ast, AstNode *node, int level) {
	
	AstNode *next = (AstNode *)radix_tree_get_next(&node->children, NULL, 0);
	if(next) {
		do {
			unsigned char *src = ast->input->buffer + next->index;
			int length = next->length;
			unsigned char name[length+1];
			int i = 0;
			for(i; i < length; i++) {
				name[i] = src[i];
			}
			name[length] = '\0';

			unsigned char levelstr[level+1];
			for(i = 0; i < level; i++) {
				levelstr[i] = '-';
			}
			levelstr[level] = '\0';

			printf("%s> [%i] %s\n", levelstr, next->symbol, name);

			ast_print_node(ast, next, level+1);

		} while(next = ast_get_next_sibling(next));
	}
}

int ast_get_symbol(AstCursor *cur, char *name, unsigned int length) {
	return symbol_table_get(cur->ast->table, name, length)->id;
}

void ast_print(Ast* ast) {
	ast_print_node(ast, &ast->root, 0);
}
