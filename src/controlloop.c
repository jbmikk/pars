#include "controlloop.h"
#include "parsercontext.h"

#include "dbg.h"

int input_continuation_follow(const Continuation *cont, Input *input, Token *token)
{
	int ret;

	if(cont->error) {
		ret = cont->error;
		goto end;
	}
	switch(cont->transition.action->type) {
	case ACTION_START:
		input_next_token(input, &cont->transition.token, token);
		ret = 0;
		break;
	case ACTION_ACCEPT:
	case ACTION_SHIFT:
	case ACTION_DROP:
		if(token->symbol == L_EOF) {
			ret = -3;
			break;
		}
		input_next_token(input, &cont->transition.token, token);
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

int control_loop_linear(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;
	Token token;
	// TODO initialize transition
	Continuation cont = { .error = 0 };

	// Dummy action for initial continuation
	Action dummy;
	action_init(&dummy, ACTION_START, 0, NULL, 0, 0);

	token_init(&token, 0, 0, 0);
	token_init(&cont.transition.token, 0, 0, 0);
	cont.transition.action = &dummy;

	while(!input_continuation_follow(&cont, context->input, &token)) {
		cont = fsm_pda_loop(&context->lexer_thread, token, context->lexer_transition);
	}

	// TODO: Add error details (lexer or parser?)
	check(
		!cont.error,
		"Parser error at token "
		"index: %i with symbol: %i, length: %i",
		token.index, token.symbol, token.length
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
			cont = fsm_pda_loop(&context->thread, token_down, context->parser_transition);
			check(
				!cont.error,
				"Parser error at node %p - DOWN", node
			);
		} else if(cursor.offset < 0) {
			cont = fsm_pda_loop(&context->thread, token_up, context->parser_transition);
			check(
				!cont.error,
				"Parser error at node %p - UP", node
			);
		}

		token_init(&token, index, 0, node->token.symbol);

		// TODO: no need for a full pda loop for now, a simple fsm
		// should do.
		cont = fsm_pda_loop(&context->thread, token, context->parser_transition);
		check(
			!cont.error,
			"Parser error at node %p - "
			"index: %i with symbol: %i, length: %i", node,
			node->token.index, node->token.symbol, node->token.length
		);

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

