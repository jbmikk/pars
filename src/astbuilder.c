#include "astbuilder.h"
#include "cmemory.h"

#include <stdlib.h>
#include <stdio.h>

#ifdef AST_TRACE
#define trace(N, A, T, I, L) printf("ast: %p, %s: %c (%i), at: %i, length: %i \n", N, A, (char)T, T, I, L);
#else
#define trace(N, A, T, I, L)
#endif


static void _node_bind_to_parent(AstNode *node)
{
	BMapEntryAstNode *pre = bmap_astnode_get(&node->parent->children, node->token.index);
	// TODO: Review algorithm: should avoid creating duplicate nodes when
	// dropping nonterminals and then closing previous nodes.
	if(pre) {
		ast_node_dispose(pre->astnode);
		free(pre->astnode);
		bmap_astnode_delete(&node->parent->children, node->token.index);
	}
	bmap_astnode_insert(&node->parent->children, node->token.index, node);
}

static AstNode *_node_append(AstBuilder *builder, const Token *token)
{
	AstNode *node = malloc(sizeof(AstNode));
	ast_node_init(node, builder->current, token);

	_node_bind_to_parent(node);

	trace(node, "add", token->symbol, token->index, token->length);
	return node;
}

void ast_builder_init(AstBuilder *builder, Ast *ast)
{
	builder->ast = ast;
	builder->current = &ast->root;
	builder->previous = NULL;
}

void ast_builder_dispose(AstBuilder *builder)
{
	//TODO Write tests for ast disposal
	while(builder->current && builder->current != &builder->ast->root) {
		Token token;
		token_init(&token, 0, 0, 0, 0);
		//TODO: What if we are not using the fsm api?
		ast_builder_reduce(builder, &token);
	}
	ast_builder_done(builder);

	builder->ast = NULL;
	builder->current = NULL;
	builder->previous = NULL;
}

void ast_builder_append(AstBuilder *builder, const Token *token)
{
	_node_append(builder, token);
}

void ast_builder_append_follow(AstBuilder *builder, const Token *token)
{
	builder->current = _node_append(builder, token);
}

void ast_builder_parent(AstBuilder *builder)
{
	builder->current = builder->current->parent;
}

void ast_builder_drop(void *builder_p, const Token *token)
{
	AstBuilder *builder = (AstBuilder *)builder_p;
	_node_append(builder, token);
}

void ast_builder_shift(void *builder_p, const Token *token)
{
	AstBuilder *builder = (AstBuilder *)builder_p;
	AstNode *node = malloc(sizeof(AstNode));
	ast_node_init(node, builder->current, &(Token){token->index, 0, 0, 0});

	trace(node, "open", '?', token->index, 0);
	AstNode *previous = builder->previous;
	if(previous != NULL) {
		builder->previous = NULL;
		trace(previous, "previous", '?', previous->token.index, 0);
		if(previous->token.index == node->token.index) {
			previous->parent = node;
		}
		_node_bind_to_parent(previous);
	}
	builder->current = node;

	if(previous == NULL || previous->token.index != node->token.index) {
		//Only add shifted symbol if it's not the same
		//that was just closed.
		_node_append(builder, token);
	}
}

void ast_builder_reduce(void *builder_p, const Token *token)
{
	AstBuilder *builder = (AstBuilder *)builder_p;
	AstNode *node = builder->current;

	node->token.length = token->length;
	node->token.symbol = token->symbol;

	trace(node, "close", token->symbol, token->index, token->length);
	if(builder->previous != NULL) {
		_node_bind_to_parent(builder->previous);
	}
	builder->previous = node;
	builder->current = node->parent;
}

void ast_builder_done(AstBuilder *builder)
{
	if(builder->previous != NULL) {
		trace(builder->previous, "done", 0, builder->previous->token.index, 0);
		_node_bind_to_parent(builder->previous);
		builder->previous = NULL;
	} else {
		//TODO: warning or sentinel?
	}
	builder->current = &builder->ast->root;
}

