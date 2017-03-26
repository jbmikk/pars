#include "atl_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"

#define nzs(S) (S), (strlen(S))

void atl_init_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	int IDENTIFIER = fsm_get_symbol_id(fsm, nzs("identifier"));
	int START_BLOCK_SYMBOL = fsm_get_symbol_id(fsm, nzs("start_block_symbol"));
	int END_BLOCK_SYMBOL = fsm_get_symbol_id(fsm, nzs("end_block_symbol"));

	//Atl Rule
	fsm_builder_define(&builder, nzs("atl_rule"));
	fsm_builder_terminal(&builder, IDENTIFIER);
	fsm_builder_terminal(&builder, START_BLOCK_SYMBOL);
	fsm_builder_terminal(&builder, END_BLOCK_SYMBOL);
	fsm_builder_end(&builder);

	//Syntax
	fsm_builder_define(&builder, nzs("syntax"));
	fsm_builder_nonterminal(&builder,  nzs("atl_rule"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("atl_rule"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

void atl_init_lexer_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	fsm_builder_set_mode(&builder, nzs(".default"));

	//White space
	//TODO: Add other white space characters
	fsm_builder_define(&builder, nzs("white_space"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal(&builder, ' ');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\t');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\n');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\r');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\f');
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, ' ');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\t');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\n');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\r');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\f');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Meta identifiers
	fsm_builder_define(&builder, nzs("identifier"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Integer
	fsm_builder_define(&builder, nzs("integer"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Terminal string
	//TODO: add all utf8 ranges
	fsm_builder_define(&builder, nzs("string"));
	fsm_builder_terminal(&builder, '"');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\\');
	fsm_builder_terminal(&builder, '"');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '"');
	fsm_builder_end(&builder);

	//Terminal string
	//TODO: add all utf8 ranges
	fsm_builder_define(&builder, nzs("string"));
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '\\');
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '\'');
	fsm_builder_end(&builder);

	//Comment
	//TODO: add white space
	fsm_builder_define(&builder, nzs("comment"));
	fsm_builder_terminal(&builder, '/');
	fsm_builder_terminal(&builder, '*');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, '*');
	//TODO: if asterisk followed by a non-asterisk it should be accepted
	fsm_builder_terminal(&builder, '/');
	fsm_builder_end(&builder);

	//Start block symbol
	fsm_builder_define(&builder, nzs("start_block_symbol"));
	fsm_builder_terminal(&builder, '{');
	fsm_builder_end(&builder);

	//End block symbol
	fsm_builder_define(&builder, nzs("end_block_symbol"));
	fsm_builder_terminal(&builder, '}');
	fsm_builder_end(&builder);

	fsm_builder_dispose(&builder);

	fsm_builder_lexer_done(&builder, L_EOF);
}

static void _atl_pipe_token(void *session, const Token *token)
{
	Session *_session = (Session *)session;
	Symbol *comment = symbol_table_get(_session->fsm->table, "comment", 7);
	Symbol *white_space = symbol_table_get(_session->fsm->table, "white_space", 11);

	//Filter white space and tokens
	if(token->symbol != comment->id && token->symbol != white_space->id) {
		session_match(_session, token);
	}
}

int atl_init_parser(Parser *parser)
{
	parser->handler.shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->handler.accept = NULL;

	parser->lexer_handler.shift = NULL;
	parser->lexer_handler.reduce = NULL;
	parser->lexer_handler.accept = _atl_pipe_token;

	symbol_table_init(&parser->table);

	fsm_init(&parser->lexer_fsm, &parser->table);
	atl_init_lexer_fsm(&parser->lexer_fsm);

	fsm_init(&parser->fsm, &parser->table);
	atl_init_fsm(&parser->fsm);

	return 0;
//error:
	//TODO: free

	//return -1;
}

int atl_dispose_parser(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	symbol_table_dispose(&parser->table);
	//TODO: handle errors?
	return 0;
}

void atl_ast_transform(Ast *ast)
{
	AstCursor a_cur;

	ast_cursor_init(&a_cur, ast);

	int ATL_RULE = ast_get_symbol(&a_cur, nzs("atl_rule"));
	while(ast_cursor_depth_next_symbol(&a_cur, ATL_RULE)) {
		ast_cursor_push(&a_cur);
		ast_cursor_pop(&a_cur);
	}


	ast_cursor_dispose(&a_cur);
}
