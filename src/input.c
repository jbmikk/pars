#include "input.h"

#include <stdlib.h>
#include <stdbool.h>

FUNCTIONS(Stack, Token, Token, token);

void input_init(Input *input)
{
	input->source = NULL;
	input->ast = NULL;
	stack_token_init(&input->token_stack);
}

void input_dispose(Input *input)
{
	stack_token_dispose(&input->token_stack);
}

void input_set_source(Input *input, Source *source)
{
	input->source = source;
}

void input_set_source_ast(Input *input, Ast *ast)
{
	input->ast = ast;
}


static void _apply_continuation(Input *input, const Continuation *cont)
{
	switch(cont->type) {
	case CONTINUATION_PUSH:
		stack_token_push(&input->token_stack, cont->token);
		stack_token_push(&input->token_stack, cont->token2);
		break;
	case CONTINUATION_RETRY:
		stack_token_push(&input->token_stack, cont->token);
		break;
	}
}

static bool _next(Input *input, Token *token)
{
	if(!stack_token_is_empty(&input->token_stack)) {
		*token = stack_token_top(&input->token_stack);
		stack_token_pop(&input->token_stack);
		return 1;
	}
	return 0;
}

Continuation input_loop(Input *input, FsmThread *thread, const Token token)
{
	Token t = token;
	Continuation cont;
	do {
		Transition transition = fsm_thread_cycle(thread, t);

		int error = fsm_thread_notify(thread, transition);

		// Listener's return value is combined with the transition to
		// get the continuation.
		cont = continuation_build(transition, error);

		_apply_continuation(input, &cont);
	} while (_next(input, &t));
	return cont;
}

int input_linear_feed(Input *input, const Continuation *cont, Token *token)
{
	int ret;

	switch(cont->type) {
	case CONTINUATION_START:
		source_next_token(input->source, &cont->token, token);
		ret = 0;
		break;
	case CONTINUATION_NEXT:
		// Is this ok? Shouldn't it be cont->token?
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}
		source_next_token(input->source, &cont->token, token);
		ret = 0;
		break;
	case CONTINUATION_ERROR:
		// TODO: error bubbling???
		// ret = cont->error;
		ret = -1;
	default:
		// TODO: sentinel? should never reach this place
		ret = -2;
		break;
	}

	return ret;
}

int input_ast_feed(Input *input, const Continuation *cont, AstCursor *cursor, Token *token)
{
	int ret;
	AstNode *node;

	switch(cont->type) {
	case CONTINUATION_START:
		node = ast_cursor_depth_next(cursor);
		token_init(token, 0, 0, node->token.symbol);

		ret = 0;
		break;
	case CONTINUATION_NEXT:
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}

		// TODO: Should the index be part of the ast?
		// We need the index for source back tracking. We should be 
		// able to move the iterator back to the specific node.
		int index = cont->token.index + 1;
		node = ast_cursor_depth_next(cursor);
		token_init(token, index, 0, node->token.symbol);
		ret = 0;
		break;
	case CONTINUATION_ERROR:
		ret = -1;
		break;
	default:
		// TODO: sentinel? should never reach this place
		ret = -2;
		break;
	}

	return ret;
}
