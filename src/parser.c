#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

int parser_execute(Parser *parser, Ast *ast, Input *input)
{
	Lexer *lexer = &parser->lexer;
	Token token;
	token_init(&token, 0, 0, 0);

	lexer_init(lexer, input, parser->lexer_fsm);
	ast_init(ast, input, &parser->table);

	Session session;
	session_init(&session, &parser->fsm);
	session_set_handler(&session, parser->handler, ast);

	do {
		lexer_next(lexer, &token);
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

