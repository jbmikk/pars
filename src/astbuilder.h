#ifndef ASTBUILDER_H
#define ASTBUILDER_H

#include "ast.h"

typedef struct _AstBuilder {
	Ast *ast;
	AstNode *current;
	AstNode *previous;
} AstBuilder;

void ast_builder_init(AstBuilder *builder, Ast *ast);
void ast_builder_dispose(AstBuilder *builder);
void ast_builder_append(AstBuilder *builder, const Token *token);
void ast_builder_append_follow(AstBuilder *builder, const Token *token);
void ast_builder_parent(AstBuilder *builder);
void ast_builder_done(AstBuilder *builder);

//Fsm api: this is where the fsm is plugged into the ast builder
void ast_builder_drop(void *builder_p, const Token *token);
void ast_builder_shift(void *builder_p, const Token *token);
void ast_builder_reduce(void *builder_p, const Token *token);

#endif //ASTBUILDER_H
