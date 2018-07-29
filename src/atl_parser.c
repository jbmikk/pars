#include "atl_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"
#include "astlistener.h"
#include "controlloop.h"
#include "parsercontext.h"

#define nzs(S) (S), (strlen(S))

void atl_build_fsm(Fsm *fsm)
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

void atl_build_lexer_fsm(Fsm *fsm)
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
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Integer
	fsm_builder_define(&builder, nzs("integer"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_group_end(&builder);
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Terminal string
	//TODO: add all utf8 ranges
	fsm_builder_define(&builder, nzs("string"));
	fsm_builder_terminal(&builder, '"');
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
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
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
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
	fsm_builder_terminal_range(&builder, (Range){'0', '9'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'z'});
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'A', 'Z'});
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

int atl_lexer_pipe(void *_context, void *_tran)
{
	ParserContext *context = (ParserContext *)_context;
	Transition *tran = (Transition *)_tran;

	if(tran->action->type != ACTION_ACCEPT) {
		return 0;
	}
	Token token = tran->token;

	Symbol *comment = symbol_table_get(&context->parser->table, "comment", 7);
	Symbol *white_space = symbol_table_get(&context->parser->table, "white_space", 11);

	Continuation cont = { .error = 0 };
	//Filter white space and tokens
	if(token.symbol != comment->id && token.symbol != white_space->id) {
		cont = fsm_thread_loop(&context->thread, token);
	}
	return cont.error;
}

void atl_build_parser_context(ParserContext *context)
{
	listener_init(&context->parse_setup_lexer, NULL, context);
	listener_init(&context->parse_setup_fsm, NULL, context);
	listener_init(&context->parse_start, ast_parse_start, context);
	listener_init(&context->parse_loop, control_loop_linear, context);
	listener_init(&context->parse_end, ast_parse_end, context);
	listener_init(&context->parse_error, ast_parse_error, context);

	listener_init(&context->lexer_pipe, atl_lexer_pipe, context);
	listener_init(&context->parser_pipe, ast_parser_pipe, context);
}

int atl_build_parser(Parser *parser)
{

	atl_build_lexer_fsm(&parser->lexer_fsm);
	atl_build_fsm(&parser->fsm);

	parser->build_context = &atl_build_parser_context;

	return 0;
//error:
	//TODO: free

	//return -1;
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
