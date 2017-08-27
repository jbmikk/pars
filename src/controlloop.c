#include "controlloop.h"

#include "parsercontext.h"
#include "dbg.h"


int control_loop_linear(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;
	token_init(&token, 0, 0, 0);

	do {
		input_next_token(context->input, &token, &token);
		fsm_thread_match(&context->lexer_thread, &token);

		check(
			context->lexer_thread.status != FSM_THREAD_ERROR,
			"Lexer error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

		check(
			context->thread.status != FSM_THREAD_ERROR,
			"Parser error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

	} while(token.symbol != L_EOF);

	return 0;
error:
	return -1;
}

