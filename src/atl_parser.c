#include "atl_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"

#define nzs(S) (S), (strlen(S))

void atl_init_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	//Atl Rule
	fsm_builder_define(&builder, nzs("atl_rule"));
	fsm_builder_terminal(&builder, ATL_IDENTIFIER);
	fsm_builder_terminal(&builder, ATL_START_BLOCK_SYMBOL);
	fsm_builder_terminal(&builder, ATL_END_BLOCK_SYMBOL);
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

int atl_init_parser(Parser *parser)
{
	parser->handler.shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->lexer_fsm = atl_lexer;

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
