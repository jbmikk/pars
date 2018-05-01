#include "controlloop.h"
#include "parsercontext.h"

#include "dbg.h"

int input_continuation_follow(const Continuation *cont, Input *input, Token *token)
{
	int ret = 0;

	switch(cont->action->type) {
	case ACTION_START:
		input_next_token(input, &cont->token, token);
		break;
	case ACTION_ACCEPT:
	case ACTION_SHIFT:
	case ACTION_DROP:
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}
		input_next_token(input, &cont->token, token);
		break;
	case ACTION_ERROR:
		ret = -1;
	default:
		// TODO: sentinel? should never reach this place
		ret = -2;
		break;
	}
	return ret;
}

int control_loop_linear(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;
	Continuation cont;

	// Dummy action for initial continuation
	Action dummy;
	action_init(&dummy, ACTION_START, 0, NULL, 0, 0);

	token_init(&token, 0, 0, 0);
	token_init(&cont.token, 0, 0, 0);
	cont.action = &dummy;

	while(!input_continuation_follow(&cont, context->input, &token)) {

		Token retry = token;
		int count = 0;
		do {
			cont = fsm_thread_match(&context->lexer_thread, &retry);

			// TODO: check errors
			listener_notify(&context->lexer_transition, &cont);

			// TODO: Add error details (lexer or parser?)
			check(
				context->output.status == OUTPUT_DEFAULT,
				"Parser error at token "
				"index: %i with symbol: %i, length: %i",
				token.index, token.symbol, token.length
			);

			// TODO: Temporary continuation, it should be in control loop
		} while (!pda_continuation_follow(&cont, &token, &retry, &count));
	}

	return 0;
error:
	return -1;
}

int control_loop_ast(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;
	token_init(&token, 0, 0, 0);

	unsigned int index = 0;

	Symbol *t_down = symbol_table_get(&context->parser->table, "__tdown", 7);
	Symbol *t_up = symbol_table_get(&context->parser->table, "__tup", 5);

	Token token_down;
	token_init(&token_down, 0, 0, t_down->id);
	Token token_up;
	token_init(&token_up, 0, 0, t_up->id);

	AstCursor cursor;
	ast_cursor_init(&cursor, context->input_ast);

	AstNode *node = ast_cursor_depth_next(&cursor);
	Continuation cont;

	while(node) {

		if(cursor.offset == 1) {
			//TODO: wrap in PDA loop
			cont = fsm_thread_match(&context->thread, &token_down);
			check(
				context->output.status == OUTPUT_DEFAULT,
				"Parser error at node %p - DOWN", node
			);
			listener_notify(&context->parser_transition, &cont);
		} else if(cursor.offset < 0) {
			//TODO: wrap in PDA loop
			cont = fsm_thread_match(&context->thread, &token_up);
			check(
				context->output.status == OUTPUT_DEFAULT,
				"Parser error at node %p - UP", node
			);
			listener_notify(&context->parser_transition, &cont);
		}

		token_init(&token, index, 0, node->token.symbol);

		//TODO: wrap in PDA loop
		cont = fsm_thread_match(&context->thread, &token);
		check(
			context->output.status == OUTPUT_DEFAULT,
			"Parser error at node %p - "
			"index: %i with symbol: %i, length: %i", node,
			node->token.index, node->token.symbol, node->token.length
		);
		listener_notify(&context->parser_transition, &cont);

		// TODO: Should the index be part of the ast?
		// We need the index for input back tracking. We should be 
		// able to move the iterator back to the specific node.
		index++;

		node = ast_cursor_depth_next(&cursor);
	}
	ast_cursor_dispose(&cursor);

	return 0;
error:
	ast_cursor_dispose(&cursor);

	return -1;
}

