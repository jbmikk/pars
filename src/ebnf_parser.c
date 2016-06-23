#include "ebnf_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"

#include <setjmp.h>

jmp_buf on_error;

void parse_error(Input *input, unsigned int index)
{
	longjmp(on_error, 1);
}

void ebnf_init_fsm(Fsm *fsm)
{
	FsmCursor cur;

	fsm_cursor_init(&cur, fsm);

	//Expression
	fsm_cursor_define(&cur, nzs("expression"));
	fsm_cursor_push(&cur);
	fsm_cursor_push_new_continuation(&cur);

	fsm_cursor_add_shift(&cur, L_IDENTIFIER);
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_reset(&cur);

	fsm_cursor_add_shift(&cur, L_TERMINAL_STRING);
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_reset(&cur);

	fsm_cursor_add_shift(&cur, L_START_GROUP_SYMBOL);
	fsm_cursor_add_reference(&cur,  nzs("definitions_list"));
	fsm_cursor_add_shift(&cur, L_END_GROUP_SYMBOL);
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_pop_continuation(&cur);
	fsm_cursor_pop(&cur);

	//Single Definition
	fsm_cursor_define(&cur, nzs("single_definition"));
	fsm_cursor_add_reference(&cur,  nzs("expression"));
	fsm_cursor_push_continuation(&cur);
	fsm_cursor_push(&cur);
	fsm_cursor_add_shift(&cur, L_CONCATENATE_SYMBOL);
	fsm_cursor_add_reference(&cur,  nzs("expression"));
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_pop_continuation(&cur);
	fsm_cursor_pop(&cur);
	fsm_cursor_set_end(&cur);

	//Definitions List
	fsm_cursor_define(&cur, nzs("definitions_list"));
	fsm_cursor_add_reference(&cur,  nzs("single_definition"));
	fsm_cursor_push_continuation(&cur);
	fsm_cursor_push(&cur);
	fsm_cursor_add_shift(&cur, L_DEFINITION_SEPARATOR_SYMBOL);
	fsm_cursor_add_reference(&cur,  nzs("single_definition"));
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_pop_continuation(&cur);
	fsm_cursor_pop(&cur);
	fsm_cursor_set_end(&cur);

	//Non Terminal Declaration
	fsm_cursor_define(&cur, nzs("non_terminal_declaration"));
	fsm_cursor_add_shift(&cur, L_IDENTIFIER);
	fsm_cursor_add_shift(&cur, L_DEFINING_SYMBOL);
	fsm_cursor_add_reference(&cur,  nzs("definitions_list"));
	fsm_cursor_add_shift(&cur, L_TERMINATOR_SYMBOL);

	//Syntax
	//TODO: Remove double call to support zero elements
	fsm_cursor_define(&cur, nzs("syntax"));
	fsm_cursor_add_reference(&cur,  nzs("non_terminal_declaration"));
	fsm_cursor_push_continuation(&cur);
	fsm_cursor_push(&cur);
	fsm_cursor_add_reference(&cur,  nzs("non_terminal_declaration"));
	fsm_cursor_join_continuation(&cur);
	fsm_cursor_pop_continuation(&cur);
	fsm_cursor_pop(&cur);
	fsm_cursor_set_end(&cur);

	fsm_cursor_done(&cur, L_EOF);

	fsm_cursor_dispose(&cur);
}

int ebnf_init_parser(Parser *parser)
{
	parser->handler.context_shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->lexer_handler = ebnf_lexer;

	symbol_table_init(&parser->table);
	fsm_init(&parser->fsm, &parser->table);
	ebnf_init_fsm(&parser->fsm);

	return 0;
error:
	//TODO: free

	return -1;
}

int ebnf_dispose_parser(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	symbol_table_dispose(&parser->table);
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur);

void ebnf_build_expression(FsmCursor *f_cur, AstCursor *a_cur)
{
	AstNode *node = ast_cursor_depth_next(a_cur);
	unsigned char *string;
	int length, i;

	int E_DEFINITIONS_LIST = ast_get_symbol(a_cur, nzs("definitions_list"));
	switch(node->symbol) {
	case L_IDENTIFIER:
		ast_cursor_get_string(a_cur, &string, &length);
		fsm_cursor_add_reference(f_cur, string, length);
		break;
	case L_TERMINAL_STRING:
		ast_cursor_get_string(a_cur, &string, &length);
		for(i = 1; i < length-1; i++) {
			//TODO: literal strings should be tokenized into simbols (utf8)
			fsm_cursor_add_shift(f_cur, string[i]);
		}
		break;
	case '(':
		ast_cursor_depth_next_symbol(a_cur, E_DEFINITIONS_LIST);
		fsm_cursor_push_new_continuation(f_cur);
		ebnf_build_definitions_list(f_cur, a_cur);
		fsm_cursor_pop_continuation(f_cur);
		break;
	default:
		//TODO:sentinel??
		break;
	}
}

void ebnf_build_single_definition(FsmCursor *f_cur, AstCursor *a_cur)
{
	int E_EXPRESSION = ast_get_symbol(a_cur, nzs("expression"));
	ast_cursor_depth_next_symbol(a_cur, E_EXPRESSION);
	do {
		ast_cursor_push(a_cur);
		ebnf_build_expression(f_cur, a_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_EXPRESSION));
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur)
{
	int E_SINGLE_DEFINITION = ast_get_symbol(a_cur, nzs("single_definition"));
	ast_cursor_depth_next_symbol(a_cur, E_SINGLE_DEFINITION);
	fsm_cursor_push(f_cur);
	do {
		ast_cursor_push(a_cur);
		ebnf_build_single_definition(f_cur, a_cur);
		fsm_cursor_join_continuation(f_cur);
		fsm_cursor_reset(f_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_SINGLE_DEFINITION));
	fsm_cursor_pop_discard(f_cur);
}

void ebnf_build_non_terminal_declaration(FsmCursor *f_cur, AstCursor *a_cur)
{
	unsigned char *string;
	int length;

	ast_cursor_depth_next_symbol(a_cur, L_IDENTIFIER);
	ast_cursor_get_string(a_cur, &string, &length);
	fsm_cursor_define(f_cur, string, length);

	int E_DEFINITIONS_LIST = ast_get_symbol(a_cur, nzs("definitions_list"));
	ast_cursor_next_sibling_symbol(a_cur, E_DEFINITIONS_LIST);
	fsm_cursor_push_new_continuation(f_cur);
	ebnf_build_definitions_list(f_cur, a_cur);
	fsm_cursor_pop_continuation(f_cur);
}	

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor a_cur;
	FsmCursor f_cur;

	ast_cursor_init(&a_cur, ast);
	fsm_cursor_init(&f_cur, fsm);

	int E_NON_TERMINAL_DECLARATION = ast_get_symbol(&a_cur, nzs("non_terminal_declaration"));
	while(ast_cursor_depth_next_symbol(&a_cur, E_NON_TERMINAL_DECLARATION)) {
		ast_cursor_push(&a_cur);
		ebnf_build_non_terminal_declaration(&f_cur, &a_cur);
		ast_cursor_pop(&a_cur);
	}

	fsm_cursor_done(&f_cur, L_EOF);
	//ast_done?

	fsm_cursor_dispose(&f_cur);
	ast_cursor_dispose(&a_cur);
	
}
