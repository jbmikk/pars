#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include "input.h"
#include "source.h"
#include "ast.h"
#include "fsmthread.h"

typedef struct Input {
	Source *source;
	Ast *ast;
} Input;

void input_init(Input *input);
void input_set_source(Input *input, Source *source);
void input_set_source_ast(Input *input, Ast *ast);
void input_dispose(Input *input);
Continuation input_loop(Input *input, FsmThread *thread, const Token token);
int input_linear_feed(Input *input, const Continuation *cont, Token *token);
int input_ast_feed(Input *input, const Continuation *cont, AstCursor *cursor, Token *token);

#endif // INPUT_H
