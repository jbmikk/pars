#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

static void _default_pipe_token(void *session, const Token *token)
{
	session_match((Session *)session, token);
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

