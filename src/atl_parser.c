#include "atl_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"

void atl_init_fsm(Fsm *fsm)
{
	FsmCursor cur;

	fsm_cursor_init(&cur, fsm);

	//Atl Rule
	fsm_cursor_define(&cur, nzs("atl_rule"));
	fsm_cursor_terminal(&cur, ATL_IDENTIFIER);
	fsm_cursor_terminal(&cur, ATL_START_BLOCK_SYMBOL);
	fsm_cursor_terminal(&cur, ATL_END_BLOCK_SYMBOL);
	fsm_cursor_end(&cur);

	//Syntax
	fsm_cursor_define(&cur, nzs("syntax"));
	fsm_cursor_nonterminal(&cur,  nzs("atl_rule"));
	fsm_cursor_loop_group_start(&cur);
	fsm_cursor_nonterminal(&cur,  nzs("atl_rule"));
	fsm_cursor_loop_group_end(&cur);
	fsm_cursor_end(&cur);

	fsm_cursor_done(&cur, L_EOF);

	fsm_cursor_dispose(&cur);
}

int atl_init_parser(Parser *parser)
{
	parser->handler.context_shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->lexer_handler = atl_lexer;

	symbol_table_init(&parser->table);
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
