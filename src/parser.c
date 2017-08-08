#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

static void _default_pipe_token(void *session, const Token *token)
{
	session_match((Session *)session, token);
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

	//Parser session
	//Copy handler prototype and set target
	FsmHandler handler = parser->handler;
	handler.target = ast;

	Session session;
	session_init(&session, &parser->fsm, handler);

	//Lexer session
	FsmHandler lexer_handler = parser->lexer_handler;
	lexer_handler.target = &session;
	if(!lexer_handler.accept) {
		lexer_handler.accept = _default_pipe_token;
	}

	Session lexer_session;
	session_init(&lexer_session, &parser->lexer_fsm, lexer_handler);

	do {
		input_next_token(input, &token, &token);
		session_match(&lexer_session, &token);

		check(
			lexer_session.status != SESSION_ERROR,
			"Lexer error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

		check(
			session.status != SESSION_ERROR,
			"Parser error at token "
			"index: %i with symbol: %i, length: %i",
			token.index, token.symbol, token.length
		);

	} while(token.symbol != L_EOF);

	ast_done(ast);
	session_dispose(&session);
	session_dispose(&lexer_session);

	return 0;
error:
	//TODO: free
	session_dispose(&session);
	session_dispose(&lexer_session);
	ast_dispose(ast);

	return -1;
}

