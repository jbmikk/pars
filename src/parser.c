#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

static void _default_pipe_token(void *thread, const Token *token)
{
	fsm_thread_match((FsmThread *)thread, token);
}

void parser_init(Parser *parser)
{
	symbol_table_init(&parser->table);
	fsm_init(&parser->lexer_fsm, &parser->table);
	fsm_init(&parser->fsm, &parser->table);

	parser_set_handlers(parser, NULL, NULL, NULL);
	parser_set_lexer_handlers(parser, NULL, NULL, NULL);
}

void parser_dispose(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	fsm_dispose(&parser->lexer_fsm);
	symbol_table_dispose(&parser->table);
}

void parser_set_handlers(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->handler.target = NULL;
	parser->handler.shift = shift;
	parser->handler.reduce = reduce;
	parser->handler.accept = accept;
}

void parser_set_lexer_handlers(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->lexer_handler.target = NULL;
	parser->lexer_handler.shift = shift;
	parser->lexer_handler.reduce = reduce;
	parser->lexer_handler.accept = accept;
}

int parser_execute(Parser *parser, Ast *ast, Input *input)
{
	Token token;
	token_init(&token, 0, 0, 0);

	ast_init(ast, input, &parser->table);

	//Parser thread
	//Copy handler prototype and set target
	FsmHandler handler = parser->handler;
	handler.target = ast;

	FsmThread thread;
	fsm_thread_init(&thread, &parser->fsm, handler);

	//Lexer thread
	FsmHandler lexer_handler = parser->lexer_handler;
	lexer_handler.target = &thread;
	if(!lexer_handler.accept) {
		lexer_handler.accept = _default_pipe_token;
	}

	FsmThread lexer_thread;
	fsm_thread_init(&lexer_thread, &parser->lexer_fsm, lexer_handler);

	do {
		input_next_token(input, &token, &token);
		fsm_thread_match(&lexer_thread, &token);

		check(
			lexer_thread.status != FSM_THREAD_ERROR,
			"Lexer error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

		check(
			thread.status != FSM_THREAD_ERROR,
			"Parser error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

	} while(token.symbol != L_EOF);

	ast_done(ast);
	fsm_thread_dispose(&thread);
	fsm_thread_dispose(&lexer_thread);

	return 0;
error:
	//TODO: free
	fsm_thread_dispose(&thread);
	fsm_thread_dispose(&lexer_thread);
	ast_dispose(ast);

	return -1;
}

