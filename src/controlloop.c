#include "controlloop.h"
#include "parsercontext.h"

#include "dbg.h"

int control_loop_linear(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;

	Continuation cont;
	token_init(&token, 0, 0, 0);
	token_init(&cont.token, 0, 0, 0);
	token_init(&cont.token2, 0, 0, 0);
	cont.type = CONTINUATION_START;

	while(!input_linear_feed(&context->input, &cont, &token)) {
		cont = input_loop(&context->input, &context->lexer_thread, token);
	}

	// TODO: Add error details (lexer or parser?)
	check(
		cont.type != CONTINUATION_ERROR,
		"Parser error at token "
		"index: %i with symbol: %i (%c), length: %i",
		token.index, token.symbol, token.symbol, token.length
	);

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

	// TODO: Move cursor inside input
	AstCursor cursor;
	ast_cursor_init(&cursor, context->input.ast);

	Continuation cont;
	token_init(&cont.token, 0, 0, 0);
	token_init(&cont.token2, 0, 0, 0);
	cont.type = CONTINUATION_START;

	while(!input_ast_feed(&context->input, &cont, &cursor, &token)) {

		// TODO: Maybe the up and down tokens should be pushed in the
		// ast feed function;
		if(cursor.offset == 1) {
			cont = input_loop(&context->input, &context->thread, token_down);
			check(
				cont.type != CONTINUATION_ERROR,
				"Parser error at DOWN node"
			);
		} else if(cursor.offset < 0) {
			cont = input_loop(&context->input, &context->thread, token_up);
			check(
				cont.type != CONTINUATION_ERROR,
				"Parser error at UP node"
			);
		}

		// TODO: no need for a full pda loop for now, a simple fsm
		// should do.
		cont = input_loop(&context->input, &context->thread, token);
		check(
			cont.type != CONTINUATION_ERROR,
			"Parser error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);
	}
	ast_cursor_dispose(&cursor);

	return 0;
error:
	ast_cursor_dispose(&cursor);

	return -1;
}

