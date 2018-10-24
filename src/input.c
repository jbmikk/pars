#include "input.h"

#include <stdlib.h>

void input_init(Input *input)
{
	input->source = NULL;
	input->ast = NULL;
}

void input_dispose(Input *input)
{
}

void input_set_source(Input *input, Source *source)
{
	input->source = source;
}

void input_set_source_ast(Input *input, Ast *ast)
{
	input->ast = ast;
}


static int _continuation_feed(Input *input, const Continuation *cont, const Token *in, Token *out, int *count)
{
	// TODO: Add constants for errors / control codes
	int ret;

	switch(cont->transition.action->type) {
	case ACTION_SHIFT:
	case ACTION_ACCEPT:
	case ACTION_DROP:
		if(*count == 0) {
			// In token was matched, end loop
			ret = 1;
		} else {
			// In token generated a reduction the last time
			// Try matching it again.
			(*count)--;
			*out = *in;
			ret = cont->error;
		}
		break;
	case ACTION_REDUCE:
		(*count)++;
		*out = cont->transition.token;
		ret = cont->error;
		break;
	case ACTION_EMPTY:
		*out = cont->transition.token;
		ret = cont->error;
		break;
	case ACTION_ERROR:
		ret = -1;
		break;
	default:
		ret = -2;
		break;
	}
	return ret;
}

Continuation input_loop(Input *input, FsmThread *thread, const Token token)
{
	int count = 0;

	Token retry = token;
	Continuation cont;
	do {
		cont = fsm_thread_cycle(thread, retry);
	} while (!_continuation_feed(input, &cont, &token, &retry, &count));
	return cont;
}

int input_linear_feed(Input *input, const Continuation *cont, Token *token)
{
	int ret;

	if(cont->error) {
		ret = cont->error;
		goto end;
	}
	switch(cont->transition.action->type) {
	case ACTION_START:
		source_next_token(input->source, &cont->transition.token, token);
		ret = 0;
		break;
	case ACTION_ACCEPT:
	case ACTION_SHIFT:
	case ACTION_DROP:
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}
		source_next_token(input->source, &cont->transition.token, token);
		ret = 0;
		break;
	case ACTION_ERROR:
		ret = -1;
	default:
		// TODO: sentinel? should never reach this place
		ret = -2;
		break;
	}

end:
	return ret;
}

int input_ast_feed(Input *input, const Continuation *cont, AstCursor *cursor, Token *token)
{
	int ret;
	AstNode *node;

	if(cont->error) {
		ret = cont->error;
		goto end;
	}
	switch(cont->transition.action->type) {
	case ACTION_START:
		node = ast_cursor_depth_next(cursor);
		token_init(token, 0, 0, node->token.symbol);

		ret = 0;
		break;
	case ACTION_ACCEPT:
	case ACTION_SHIFT:
	case ACTION_DROP:
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}

		// TODO: Should the index be part of the ast?
		// We need the index for source back tracking. We should be 
		// able to move the iterator back to the specific node.
		int index = cont->transition.token.index + 1;
		node = ast_cursor_depth_next(cursor);
		token_init(token, index, 0, node->token.symbol);
		ret = 0;
		break;
	case ACTION_ERROR:
		ret = -1;
	default:
		// TODO: sentinel? should never reach this place
		ret = -2;
		break;
	}

end:
	return ret;
}
