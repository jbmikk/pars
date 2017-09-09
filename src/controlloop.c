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

int control_loop_ast(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;
	token_init(&token, 0, 0, 0);

	Symbol *t_down = symbol_table_get(&context->parser->table, "__tdown", 7);
	Symbol *t_up = symbol_table_get(&context->parser->table, "__tup", 5);

	Token token_down;
	token_init(&token_down, 0, 0, t_down->id);
	Token token_up;
	token_init(&token_up, 0, 0, t_up->id);

	AstCursor cursor;
	ast_cursor_init(&cursor, context->input_ast);

	AstNode *node = ast_cursor_depth_next(&cursor);
	while(node) {

		if(cursor.offset == 1) {
			fsm_thread_match(&context->thread, &token_down);
			check(
				context->thread.status != FSM_THREAD_ERROR,
				"Parser error at node %p - DOWN", node
			);
		} else if(cursor.offset < 0) {
			fsm_thread_match(&context->thread, &token_up);
			check(
				context->thread.status != FSM_THREAD_ERROR,
				"Parser error at node %p - UP", node
			);
		}

		fsm_thread_match(&context->thread, &node->token);

		check(
			context->thread.status != FSM_THREAD_ERROR,
			"Parser error at node %p - "
			"index: %i with symbol: %i, length: %i", node,
			node->token.index, node->token.symbol, node->token.length
		);

		node = ast_cursor_depth_next(&cursor);
	}
	ast_cursor_dispose(&cursor);

	return 0;
error:
	ast_cursor_dispose(&cursor);

	return -1;
}

