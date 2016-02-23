#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

int parser_execute(Parser *parser, Ast *ast, Input *input)
{
	Lexer *lexer = &parser->lexer;
	Fsm *fsm = &parser->fsm;

	lexer_init(lexer, input, parser->lexer_handler);
	ast_init(ast, input);

	Session session;
	session_init(&session, fsm);
	session_set_handler(&session, parser->handler, ast);

	while (!input->eof) {
		lexer_next(lexer);
		session_match(&session, lexer->symbol, lexer->index, lexer->length);
		check(
			session.current->type != ACTION_TYPE_ERROR,
			"Error parsing grammar at index: %i with symbol: %i, length: %i",
			session.index, lexer->symbol, lexer->length
		);
	}

	ast_done(ast);
	session_dispose(&session);

	return 0;
error:
	//TODO: free
	session_dispose(&session);
	ast_dispose(ast);

	return -1;
}

