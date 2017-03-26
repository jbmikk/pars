#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

int parser_execute(Parser *parser, Ast *ast, Input *input)
{
	Lexer *lexer = &parser->lexer;
	Token token;
	token_init(&token, 0, 0, 0);

	lexer_init(lexer, input);
	ast_init(ast, input, &parser->table);

	//Copy handler prototype and set target
	FsmHandler handler = parser->handler;
	handler.target = ast;

	Session session;
	session_init(&session, &parser->fsm, handler);

	do {
		parser->lexer_fsm(lexer, &token, &token);
		session_match(&session, &token);
		check(
			session.status != SESSION_ERROR,
			"Error parsing input at index: %i with symbol: %i, length: %i",
			session.index, token.symbol, token.length
		);
	} while(token.symbol != L_EOF);

	ast_done(ast);
	session_dispose(&session);

	return 0;
error:
	//TODO: free
	session_dispose(&session);
	ast_dispose(ast);

	return -1;
}

