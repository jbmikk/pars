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
	fsm_thread_init(&parser->proto_fsm_thread, &parser->fsm);
	fsm_thread_init(&parser->proto_lexer_fsm_thread, &parser->lexer_fsm);
}

void parser_dispose(Parser *parser)
{
	fsm_thread_dispose(&parser->proto_fsm_thread);
	fsm_thread_dispose(&parser->proto_lexer_fsm_thread);
	fsm_dispose(&parser->fsm);
	fsm_dispose(&parser->lexer_fsm);
	symbol_table_dispose(&parser->table);
}

void parser_setup_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->proto_fsm_thread.handler.target = NULL;
	parser->proto_fsm_thread.handler.shift = shift;
	parser->proto_fsm_thread.handler.reduce = reduce;
	parser->proto_fsm_thread.handler.accept = accept;
}

void parser_setup_lexer_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->proto_lexer_fsm_thread.handler.target = NULL;
	parser->proto_lexer_fsm_thread.handler.shift = shift;
	parser->proto_lexer_fsm_thread.handler.reduce = reduce;
	parser->proto_lexer_fsm_thread.handler.accept = accept;
}

int parser_execute(Parser *parser, Ast *ast, Input *input)
{
	Token token;
	token_init(&token, 0, 0, 0);

	ast_init(ast, input, &parser->table);

	//Parser thread
	//Clone thread prototype and set target
	FsmThread thread = parser->proto_fsm_thread;
	thread.handler.target = ast;

	//Close lexer thread
	FsmThread lexer_thread = parser->proto_lexer_fsm_thread;
	lexer_thread.handler.target = &thread;
	if(!lexer_thread.handler.accept) {
		lexer_thread.handler.accept = _default_pipe_token;
	}

	fsm_thread_start(&thread);
	fsm_thread_start(&lexer_thread);
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

