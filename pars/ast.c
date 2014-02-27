#include "ast.h"

#include "cmemory.h"

void ast_init(Ast *ast)
{
	//ast_node_init(&ast->root);
	ast->current = NULL;
	ast->previous = NULL;
}

void ast_node_init(AstNode *node, AstNode *parent, unsigned int index)
{
	NODE_INIT(node->children, 0, 0, NULL);
	node->parent = parent;
	node->index = index;
}

void ast_open(Ast *ast, unsigned int index, unsigned int length, int symbol)
{
	AstNode *node = c_new(AstNode, 1);
	//node = ast.root;
	ast_node_init(node, ast->current, index);

	if(ast->previous != NULL) {
		AstNode *previous = ast->previous;
		ast->previous = NULL;
		unsigned char buffer[sizeof(int)];
		unsigned int size;
		_symbol_to_buffer(buffer, &size, previous->index);
		if(previous->index == node->index)
			previous->parent = node;
		c_radix_tree_set(&previous->parent->children, buffer, size, previous);
	}
	ast->current = node;
}

void ast_close(Ast *ast, unsigned int index, unsigned int length, int symbol)
{
	AstNode *node = ast->current;

	ast->previous = node;
	ast->current = node->parent;
}
