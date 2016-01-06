#include "ebnf_parser.h"

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
	fsm_cursor_add_context_shift(&cur, L_IDENTIFIER);
	fsm_cursor_add_reduce(&cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_pop(&cur);
	fsm_cursor_push(&cur);
	fsm_cursor_add_context_shift(&cur, L_TERMINAL_STRING);
	fsm_cursor_add_reduce(&cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_pop(&cur);
	fsm_cursor_add_context_shift(&cur, L_START_GROUP_SYMBOL);
	fsm_cursor_push(&cur);

	//Single Definition
	fsm_cursor_define(&cur, nzs("single_definition"));
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("expression")));
	fsm_cursor_add_context_shift(&cur, E_EXPRESSION);
	fsm_cursor_push_followset(&cur);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_shift(&cur, L_CONCATENATE_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("expression")));
	fsm_cursor_add_shift(&cur, E_EXPRESSION);
	//Loop using first state followset to avoid context shift
	fsm_cursor_add_followset(&cur, fsm_cursor_pop_followset(&cur));

	//Definitions List
	fsm_cursor_define(&cur, nzs("definitions_list"));
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("single_definition")));
	fsm_cursor_add_context_shift(&cur, E_SINGLE_DEFINITION);
	fsm_cursor_push_followset(&cur);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_DEFINITION_SEPARATOR_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("single_definition")));
	fsm_cursor_add_shift(&cur, E_SINGLE_DEFINITION);
	fsm_cursor_add_followset(&cur, fsm_cursor_pop_followset(&cur));

	//Finish Expression
	fsm_cursor_pop(&cur);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("definitions_list")));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_END_GROUP_SYMBOL);
	fsm_cursor_add_reduce(&cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);

	//Non Terminal Declaration
	fsm_cursor_define(&cur, nzs("non_terminal_declaration"));
	fsm_cursor_add_context_shift(&cur, L_IDENTIFIER);
	fsm_cursor_add_shift(&cur, L_DEFINING_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("definitions_list")));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_TERMINATOR_SYMBOL);
	fsm_cursor_add_reduce(&cur, L_IDENTIFIER, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_reduce(&cur, L_EOF, E_NON_TERMINAL_DECLARATION);

	//Syntax
	fsm_cursor_define(&cur, "syntax", 6);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("non_terminal_declaration")));
	fsm_cursor_add_context_shift(&cur, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_push_followset(&cur);
	fsm_cursor_add_reduce(&cur, L_EOF, E_SYNTAX);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, nzs("non_terminal_declaration")));
	fsm_cursor_add_shift(&cur, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_followset(&cur, fsm_cursor_pop_followset(&cur));

	//fsm_set_start(fsm, "definitions_list", 16, E_SINGLE_DEFINITION);
	fsm_cursor_set_start(&cur, "syntax", 6, E_SYNTAX);

	fsm_cursor_dispose(&cur);
}

int ebnf_fsm_ast_handler(int type, void *target, void *args) {
	Ast *ast = (Ast *)target;
	FsmArgs *red = (FsmArgs *)args;

	switch(type) {
	case EVENT_REDUCE:
		ast_close(ast, red->index, red->length, red->symbol);
		break;
	case EVENT_CONTEXT_SHIFT:
		ast_open(ast, red->index);
		break;
	}
}

int ebnf_input_to_ast(Ast *ast, Input *input)
{
	Lexer lexer;
	Fsm ebnf_fsm;

	EventListener ebnf_listener;
	ebnf_listener.target = ast;
	ebnf_listener.handler = ebnf_fsm_ast_handler;

	lexer_init(&lexer, input);
	fsm_init(&ebnf_fsm);
	ebnf_init_fsm(&ebnf_fsm);
	ast_init(ast, input);

	Session *session = fsm_start_session(&ebnf_fsm);
	session_set_listener(session, ebnf_listener);

	while (!input->eof) {
		lexer_next(&lexer);
		session_match(session, lexer.symbol, lexer.index);
		check(
			session->current->type != ACTION_TYPE_ERROR,
			"Error parsing grammar at index: %i with symbol: %i",
			session->index, lexer.symbol
		);
	}
	ast_done(ast);
	session_dispose(session);
	fsm_dispose(&ebnf_fsm);

	return 0;
error:
	//TODO: free

	return -1;
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur);

void ebnf_build_expression(FsmCursor *f_cur, AstCursor *a_cur)
{
	AstNode *node = ast_cursor_depth_next(a_cur);
	NonTerminal *non_terminal;
	unsigned char *string;
	int length, i;

	switch(node->symbol) {
	case L_IDENTIFIER:
		ast_cursor_get_string(a_cur, &string, &length);
		//TODO: Need to be able to reference yet to be defined non terminals
		non_terminal = fsm_get_non_terminal(f_cur->fsm, string, length);
		fsm_cursor_add_followset(f_cur, non_terminal->start);
		fsm_cursor_add_shift(f_cur, non_terminal->symbol);
		break;
	case L_TERMINAL_STRING:
		fsm_get_state(f_cur->fsm, string, length);
		for(i = 1; i < length-1; i++) {
			//TODO: literal strings should be tokenized into simbols (utf8)
			fsm_cursor_add_shift(f_cur, string[i]);
		}
		break;
	default:
		ast_cursor_depth_next_symbol(a_cur, E_DEFINITIONS_LIST);
		ebnf_build_definitions_list(f_cur, a_cur);
		break;
	}
}

void ebnf_build_single_definition(FsmCursor *f_cur, AstCursor *a_cur)
{
	ast_cursor_depth_next_symbol(a_cur, E_EXPRESSION);
	do {
		ast_cursor_push(a_cur);
		ebnf_build_expression(f_cur, a_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_EXPRESSION));
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur)
{
	ast_cursor_depth_next_symbol(a_cur, E_SINGLE_DEFINITION);
	do {
		ast_cursor_push(a_cur);
		fsm_cursor_push(f_cur);
		ebnf_build_single_definition(f_cur, a_cur);
		fsm_cursor_pop(f_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_SINGLE_DEFINITION));
}

void ebnf_build_non_terminal_declaration(FsmCursor *f_cur, AstCursor *a_cur)
{
	unsigned char *string;
	int length;

	ast_cursor_depth_next_symbol(a_cur, L_IDENTIFIER);
	ast_cursor_get_string(a_cur, &string, &length);
	fsm_cursor_define(f_cur, string, length);

	ast_cursor_next_sibling_symbol(a_cur, E_DEFINITIONS_LIST);
	ebnf_build_definitions_list(f_cur, a_cur);
}	

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor a_cur;
	FsmCursor f_cur;

	ast_cursor_init(&a_cur, ast);
	fsm_cursor_init(&f_cur, fsm);

	while(ast_cursor_depth_next_symbol(&a_cur, E_NON_TERMINAL_DECLARATION)) {
		ast_cursor_push(&a_cur);
		ebnf_build_non_terminal_declaration(&f_cur, &a_cur);
		ast_cursor_pop(&a_cur);
	}

	fsm_cursor_dispose(&f_cur);
	ast_cursor_dispose(&a_cur);
}
