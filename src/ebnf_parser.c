#include "ebnf_parser.h"
#include "symbols.h"

#include "cmemory.h"
#include "dbg.h"

#include <setjmp.h>

#define nzs(S) (S), (strlen(S))

jmp_buf on_error;

void parse_error(Input *input, unsigned int index)
{
	longjmp(on_error, 1);
}

void ebnf_init_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	//Syntactic Primary
	fsm_builder_define(&builder, nzs("syntactic_primary"));
	fsm_builder_group_start(&builder);

	fsm_builder_terminal(&builder, E_META_IDENTIFIER);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, E_TERMINAL_STRING);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, E_SPECIAL_SEQUENCE);
	fsm_builder_or(&builder);

	fsm_builder_nonterminal(&builder,  nzs("character_set"));
	fsm_builder_or(&builder);


	fsm_builder_terminal(&builder, E_START_GROUP_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, E_END_GROUP_SYMBOL);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, E_START_OPTION_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, E_END_OPTION_SYMBOL);
	fsm_builder_or(&builder);

	fsm_builder_terminal(&builder, E_START_REPETITION_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, E_END_REPETITION_SYMBOL);
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	//Character
	fsm_builder_define(&builder, nzs("character"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, 'a', 'z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, 'A', 'Z');
	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, '0', '9');
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	//Character primary
	fsm_builder_define(&builder, nzs("character_primary"));
	fsm_builder_nonterminal(&builder,  nzs("character"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, '-');
	fsm_builder_nonterminal(&builder,  nzs("character"));
	fsm_builder_option_group_end(&builder);
	fsm_builder_end(&builder);

	//Character set
	fsm_builder_define(&builder, nzs("character_set"));
	fsm_builder_terminal(&builder, E_START_CHARACTER_SET);
	fsm_builder_nonterminal(&builder,  nzs("character_primary"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("character_primary"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_terminal(&builder, E_END_CHARACTER_SET);
	fsm_builder_end(&builder);

	//Syntactic Factor
	fsm_builder_define(&builder, nzs("syntactic_factor"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, E_INTEGER);
	fsm_builder_terminal(&builder, E_REPETITION_SYMBOL);
	fsm_builder_option_group_end(&builder);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_primary"));
	fsm_builder_end(&builder);

	//Syntactic Exception
	fsm_builder_define(&builder, nzs("syntactic_exception"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_factor"));
	fsm_builder_end(&builder);

	//Syntactic Term
	fsm_builder_define(&builder, nzs("syntactic_term"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_factor"));
	fsm_builder_option_group_start(&builder);
	fsm_builder_terminal(&builder, E_EXCEPT_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_exception"));
	fsm_builder_option_group_end(&builder);
	fsm_builder_end(&builder);

	//Single Definition
	fsm_builder_define(&builder, nzs("single_definition"));
	fsm_builder_nonterminal(&builder,  nzs("syntactic_term"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, E_CONCATENATE_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("syntactic_term"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Definitions List
	fsm_builder_define(&builder, nzs("definitions_list"));
	fsm_builder_nonterminal(&builder,  nzs("single_definition"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_terminal(&builder, E_DEFINITION_SEPARATOR_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("single_definition"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	//Syntax Rule
	fsm_builder_define(&builder, nzs("syntax_rule"));
	fsm_builder_terminal(&builder, E_META_IDENTIFIER);
	fsm_builder_terminal(&builder, E_DEFINING_SYMBOL);
	fsm_builder_nonterminal(&builder,  nzs("definitions_list"));
	fsm_builder_terminal(&builder, E_TERMINATOR_SYMBOL);
	fsm_builder_end(&builder);

	//Syntax
	fsm_builder_define(&builder, nzs("syntax"));
	fsm_builder_nonterminal(&builder,  nzs("syntax_rule"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("syntax_rule"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

int ebnf_init_parser(Parser *parser)
{
	parser->handler.shift = ast_open;
	parser->handler.reduce = ast_close;
	parser->handler.accept = NULL;
	parser->lexer_fsm = ebnf_lexer;

	symbol_table_init(&parser->table);
	fsm_init(&parser->fsm, &parser->table);
	ebnf_init_fsm(&parser->fsm);

	return 0;
//error:
	//TODO: free

	//return -1;
}

int ebnf_dispose_parser(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	symbol_table_dispose(&parser->table);
	//TODO: handle errors?
	return 0;
}

static int parse_utf8(char *array, unsigned int size)
{
	//TODO properly parse utf8 point codes, for now only ascii.
	int code = array[0];
	return code;
}

void ebnf_build_character_set(FsmBuilder *builder, AstCursor *cur)
{
	char *string;
	int length;
	int first = 1;

	int E_CHARACTER_PRIMARY = ast_get_symbol(cur, nzs("character_primary"));
	int E_CHARACTER = ast_get_symbol(cur, nzs("character"));

	ast_cursor_depth_next_symbol(cur, E_CHARACTER_PRIMARY);
	do {
		ast_cursor_push(cur);

		if(first) {
			first = 0;
		} else {
			fsm_builder_or(builder);
		}
		ast_cursor_depth_next_symbol(cur, E_CHARACTER);

		ast_cursor_get_string(cur, &string, &length);
		int start = parse_utf8(string, length);

		if(ast_cursor_next_sibling_symbol(cur, E_CHARACTER)) {
			ast_cursor_get_string(cur, &string, &length);
			int end = parse_utf8(string, length);
			//TODO: Verify start < end
			fsm_builder_terminal_range(builder, start, end);
		} else {
			fsm_builder_terminal(builder, start);
		}
		ast_cursor_pop(cur);

	} while(ast_cursor_next_sibling_symbol(cur, E_CHARACTER_PRIMARY));
}

void ebnf_build_definitions_list(FsmBuilder *builder, AstCursor *cur);

void ebnf_build_syntactic_primary(FsmBuilder *builder, AstCursor *cur)
{
	AstNode *node = ast_cursor_depth_next(cur);
	char *string;
	int length, i;
	
	int E_CHARACTER_SET;
	int E_DEFINITIONS_LIST = ast_get_symbol(cur, nzs("definitions_list"));
	switch(node->symbol) {
	case E_META_IDENTIFIER:
		ast_cursor_get_string(cur, &string, &length);
		fsm_builder_nonterminal(builder, string, length);
		break;
	case E_TERMINAL_STRING:
		ast_cursor_get_string(cur, &string, &length);
		for(i = 1; i < length-1; i++) {
			//TODO: literal strings should be tokenized into simbols (utf8)
			fsm_builder_terminal(builder, string[i]);
		}
		break;
	case E_SPECIAL_SEQUENCE:
		//TODO: define special sequences behaviour
		log_warn("Special sequence is not defined");
		break;
	case '(':
		ast_cursor_depth_next_symbol(cur, E_DEFINITIONS_LIST);
		fsm_builder_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_group_end(builder);
		break;
	case '{':
		ast_cursor_depth_next_symbol(cur, E_DEFINITIONS_LIST);
		fsm_builder_loop_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_loop_group_end(builder);
		break;
	case '[':
		ast_cursor_depth_next_symbol(cur, E_DEFINITIONS_LIST);
		fsm_builder_option_group_start(builder);
		ebnf_build_definitions_list(builder, cur);
		fsm_builder_option_group_end(builder);
		break;
	default:
		E_CHARACTER_SET = ast_get_symbol(cur, nzs("character_set"));
		if(node->symbol == E_CHARACTER_SET) {
			fsm_builder_group_start(builder);
			ebnf_build_character_set(builder, cur);
			fsm_builder_group_end(builder);
		} else {
			//TODO:sentinel??
		}
	}
}

void ebnf_build_syntactic_factor(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_PRIMARY = ast_get_symbol(cur, nzs("syntactic_primary"));

	/*
	 * TODO: Can't use depth_next methods to probe optional children.
	 * It may jump to its grandchildren.
	if(ast_cursor_depth_next_symbol(cur, E_INTEGER)) {
		log_warn("Syntactic factors are not supported");
	}
	*/

	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_PRIMARY);
	ebnf_build_syntactic_primary(builder, cur);
}

void ebnf_build_syntactic_term(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_FACTOR = ast_get_symbol(cur, nzs("syntactic_factor"));
	int E_SYNTACTIC_EXCEPTION = ast_get_symbol(cur, nzs("syntactic_exception"));

	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_FACTOR);
	ast_cursor_push(cur);
	ebnf_build_syntactic_factor(builder, cur);
	ast_cursor_pop(cur);
	if(ast_cursor_next_sibling_symbol(cur, E_SYNTACTIC_EXCEPTION)) {
		log_warn("Syntactic exceptions are not supported");
	}
}

void ebnf_build_single_definition(FsmBuilder *builder, AstCursor *cur)
{
	int E_SYNTACTIC_TERM = ast_get_symbol(cur, nzs("syntactic_term"));
	ast_cursor_depth_next_symbol(cur, E_SYNTACTIC_TERM);
	do {
		ast_cursor_push(cur);
		ebnf_build_syntactic_term(builder, cur);
		ast_cursor_pop(cur);
	} while(ast_cursor_next_sibling_symbol(cur, E_SYNTACTIC_TERM));
}

void ebnf_build_definitions_list(FsmBuilder *builder, AstCursor *cur)
{
	int first = 1;
	int E_SINGLE_DEFINITION = ast_get_symbol(cur, nzs("single_definition"));
	ast_cursor_depth_next_symbol(cur, E_SINGLE_DEFINITION);
	do {
		ast_cursor_push(cur);
		if(first) {
			first = 0;
		} else {
			fsm_builder_or(builder);
		}
		ebnf_build_single_definition(builder, cur);
		ast_cursor_pop(cur);
	} while(ast_cursor_next_sibling_symbol(cur, E_SINGLE_DEFINITION));
}

void ebnf_build_syntax_rule(FsmBuilder *builder, AstCursor *cur)
{
	char *string;
	int length;

	//TODO: maybe we should use more precise cursor functions.
	// If the ast is badly formed we could end up reading a different node
	// than expected. This situations should be handled gracefully
	// Maybe an error could be thrown or a sentinel could be placed.
	ast_cursor_depth_next_symbol(cur, E_META_IDENTIFIER);
	ast_cursor_get_string(cur, &string, &length);
	fsm_builder_define(builder, string, length);

	int E_DEFINITIONS_LIST = ast_get_symbol(cur, nzs("definitions_list"));
	ast_cursor_next_sibling_symbol(cur, E_DEFINITIONS_LIST);
	fsm_builder_group_start(builder);
	ebnf_build_definitions_list(builder, cur);
	fsm_builder_group_end(builder);
	fsm_builder_end(builder);
}	

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor cur;
	FsmBuilder builder;

	ast_cursor_init(&cur, ast);
	fsm_builder_init(&builder, fsm);

	int E_SYNTAX_RULE = ast_get_symbol(&cur, nzs("syntax_rule"));
	while(ast_cursor_depth_next_symbol(&cur, E_SYNTAX_RULE)) {
		ast_cursor_push(&cur);
		ebnf_build_syntax_rule(&builder, &cur);
		ast_cursor_pop(&cur);
	}

	fsm_builder_done(&builder, L_EOF);
	//ast_done?

	fsm_builder_dispose(&builder);
	ast_cursor_dispose(&cur);
	
}
