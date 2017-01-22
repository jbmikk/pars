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

	//Syntactic Primary
	fsm_cursor_define(&cur, nzs("syntactic_primary"));
	fsm_cursor_group_start(&cur);

	fsm_cursor_terminal(&cur, L_META_IDENTIFIER);
	fsm_cursor_or(&cur);

	fsm_cursor_terminal(&cur, L_TERMINAL_STRING);
	fsm_cursor_or(&cur);

	fsm_cursor_terminal(&cur, L_SPECIAL_SEQUENCE);
	fsm_cursor_or(&cur);

	fsm_cursor_terminal(&cur, L_START_GROUP_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("definitions_list"));
	fsm_cursor_terminal(&cur, L_END_GROUP_SYMBOL);
	fsm_cursor_or(&cur);

	fsm_cursor_terminal(&cur, L_START_OPTION_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("definitions_list"));
	fsm_cursor_terminal(&cur, L_END_OPTION_SYMBOL);
	fsm_cursor_or(&cur);

	fsm_cursor_terminal(&cur, L_START_REPETITION_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("definitions_list"));
	fsm_cursor_terminal(&cur, L_END_REPETITION_SYMBOL);
	fsm_cursor_group_end(&cur);
	fsm_cursor_end(&cur);

	//Single Definition
	fsm_cursor_define(&cur, nzs("single_definition"));
	fsm_cursor_nonterminal(&cur,  nzs("syntactic_primary"));
	fsm_cursor_loop_group_start(&cur);
	fsm_cursor_terminal(&cur, L_CONCATENATE_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("syntactic_primary"));
	fsm_cursor_loop_group_end(&cur);
	fsm_cursor_end(&cur);

	//Definitions List
	fsm_cursor_define(&cur, nzs("definitions_list"));
	fsm_cursor_nonterminal(&cur,  nzs("single_definition"));
	fsm_cursor_loop_group_start(&cur);
	fsm_cursor_terminal(&cur, L_DEFINITION_SEPARATOR_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("single_definition"));
	fsm_cursor_loop_group_end(&cur);
	fsm_cursor_end(&cur);

	//Syntax Rule
	fsm_cursor_define(&cur, nzs("syntax_rule"));
	fsm_cursor_terminal(&cur, L_META_IDENTIFIER);
	fsm_cursor_terminal(&cur, L_DEFINING_SYMBOL);
	fsm_cursor_nonterminal(&cur,  nzs("definitions_list"));
	fsm_cursor_terminal(&cur, L_TERMINATOR_SYMBOL);
	fsm_cursor_end(&cur);

	//Syntax
	fsm_cursor_define(&cur, nzs("syntax"));
	fsm_cursor_nonterminal(&cur,  nzs("syntax_rule"));
	fsm_cursor_loop_group_start(&cur);
	fsm_cursor_nonterminal(&cur,  nzs("syntax_rule"));
	fsm_cursor_loop_group_end(&cur);
	fsm_cursor_end(&cur);

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

void ebnf_build_syntactic_primary(FsmCursor *f_cur, AstCursor *a_cur)
{
	AstNode *node = ast_cursor_depth_next(a_cur);
	unsigned char *string;
	int length, i;

	int E_DEFINITIONS_LIST = ast_get_symbol(a_cur, nzs("definitions_list"));
	switch(node->symbol) {
	case L_META_IDENTIFIER:
		ast_cursor_get_string(a_cur, &string, &length);
		fsm_cursor_nonterminal(f_cur, string, length);
		break;
	case L_TERMINAL_STRING:
		ast_cursor_get_string(a_cur, &string, &length);
		for(i = 1; i < length-1; i++) {
			//TODO: literal strings should be tokenized into simbols (utf8)
			fsm_cursor_terminal(f_cur, string[i]);
		}
		break;
	case L_SPECIAL_SEQUENCE:
		//TODO: define special sequences behaviour
		log_warn("Special sequence is not defined");
		break;
	case '(':
		ast_cursor_depth_next_symbol(a_cur, E_DEFINITIONS_LIST);
		fsm_cursor_group_start(f_cur);
		ebnf_build_definitions_list(f_cur, a_cur);
		fsm_cursor_group_end(f_cur);
		break;
	case '{':
		ast_cursor_depth_next_symbol(a_cur, E_DEFINITIONS_LIST);
		fsm_cursor_loop_group_start(f_cur);
		ebnf_build_definitions_list(f_cur, a_cur);
		fsm_cursor_loop_group_end(f_cur);
		break;
	case '[':
		ast_cursor_depth_next_symbol(a_cur, E_DEFINITIONS_LIST);
		fsm_cursor_option_group_start(f_cur);
		ebnf_build_definitions_list(f_cur, a_cur);
		fsm_cursor_option_group_end(f_cur);
		break;
	default:
		//TODO:sentinel??
		break;
	}
}

void ebnf_build_single_definition(FsmCursor *f_cur, AstCursor *a_cur)
{
	int E_SYNTACTIC_PRIMARY = ast_get_symbol(a_cur, nzs("syntactic_primary"));
	ast_cursor_depth_next_symbol(a_cur, E_SYNTACTIC_PRIMARY);
	do {
		ast_cursor_push(a_cur);
		ebnf_build_syntactic_primary(f_cur, a_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_SYNTACTIC_PRIMARY));
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur)
{
	int first = 1;
	int E_SINGLE_DEFINITION = ast_get_symbol(a_cur, nzs("single_definition"));
	ast_cursor_depth_next_symbol(a_cur, E_SINGLE_DEFINITION);
	do {
		ast_cursor_push(a_cur);
		if(first) {
			first = 0;
		} else {
			fsm_cursor_or(f_cur);
		}
		ebnf_build_single_definition(f_cur, a_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_SINGLE_DEFINITION));
}

void ebnf_build_syntax_rule(FsmCursor *f_cur, AstCursor *a_cur)
{
	unsigned char *string;
	int length;

	//TODO: maybe we should use more precise cursor functions.
	// If the ast is badly formed we could end up reading a different node
	// than expected. This situations should be handled gracefully
	// Maybe an error could be thrown or a sentinel could be placed.
	ast_cursor_depth_next_symbol(a_cur, L_META_IDENTIFIER);
	ast_cursor_get_string(a_cur, &string, &length);
	fsm_cursor_define(f_cur, string, length);

	int E_DEFINITIONS_LIST = ast_get_symbol(a_cur, nzs("definitions_list"));
	ast_cursor_next_sibling_symbol(a_cur, E_DEFINITIONS_LIST);
	fsm_cursor_group_start(f_cur);
	ebnf_build_definitions_list(f_cur, a_cur);
	fsm_cursor_group_end(f_cur);
	fsm_cursor_end(f_cur);
}	

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor a_cur;
	FsmCursor f_cur;

	ast_cursor_init(&a_cur, ast);
	fsm_cursor_init(&f_cur, fsm);

	int E_SYNTAX_RULE = ast_get_symbol(&a_cur, nzs("syntax_rule"));
	while(ast_cursor_depth_next_symbol(&a_cur, E_SYNTAX_RULE)) {
		ast_cursor_push(&a_cur);
		ebnf_build_syntax_rule(&f_cur, &a_cur);
		ast_cursor_pop(&a_cur);
	}

	fsm_cursor_done(&f_cur, L_EOF);
	//ast_done?

	fsm_cursor_dispose(&f_cur);
	ast_cursor_dispose(&a_cur);
	
}
