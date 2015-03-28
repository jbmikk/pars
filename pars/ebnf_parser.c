#include "ebnf_parser.h"

#include "cmemory.h"

#include <setjmp.h>

jmp_buf on_error;

void parse_error(Input *input, unsigned int index)
{
	longjmp(on_error, 1);
}

void ebnf_init_fsm(Fsm *fsm)
{
	FsmCursor cur;
	State *ref;

	fsm_init(fsm);

	fsm_cursor_init(&cur, fsm);

	//Expression
	fsm_cursor_set(&cur, "expression", 10);
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
	fsm_cursor_set(&cur, "single_definition", 17);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "expression", 10));
	fsm_cursor_add_context_shift(&cur, E_EXPRESSION);
	ref = cur.current;
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_shift(&cur, L_CONCATENATE_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "expression", 10));
	fsm_cursor_add_shift(&cur, E_EXPRESSION);
	//Loop using first state followset to avoid context shift
	fsm_cursor_add_followset(&cur, ref);

	//Definitions List
	fsm_cursor_set(&cur, "definitions_list", 16);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "single_definition", 17));
	fsm_cursor_add_context_shift(&cur, E_SINGLE_DEFINITION);
	ref = cur.current;
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_DEFINITION_SEPARATOR_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "single_definition", 17));
	fsm_cursor_add_shift(&cur, E_SINGLE_DEFINITION);
	fsm_cursor_add_followset(&cur, ref);

	//Finish Expression
	fsm_cursor_pop(&cur);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "definitions_list", 16));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_END_GROUP_SYMBOL);
	fsm_cursor_add_reduce(&cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);

	//Non Terminal Declaration
	fsm_cursor_set(&cur, "non_terminal_declaration", 24);
	fsm_cursor_add_context_shift(&cur, L_IDENTIFIER);
	fsm_cursor_add_shift(&cur, L_DEFINING_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "definitions_list", 16));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_TERMINATOR_SYMBOL);
	fsm_cursor_add_reduce(&cur, L_IDENTIFIER, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_reduce(&cur, L_EOF, E_NON_TERMINAL_DECLARATION);

	//Syntax
	fsm_cursor_set(&cur, "syntax", 6);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "non_terminal_declaration", 24));
	fsm_cursor_add_context_shift(&cur, E_NON_TERMINAL_DECLARATION);
	ref = cur.current;
	fsm_cursor_add_reduce(&cur, L_EOF, E_SYNTAX);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "non_terminal_declaration", 24));
	fsm_cursor_add_shift(&cur, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_followset(&cur, ref);

	//fsm_set_start(fsm, "definitions_list", 16, E_SINGLE_DEFINITION);
	fsm_cursor_set_start(&cur, "syntax", 6, E_SYNTAX);
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

void ebnf_input_to_ast(Ast *ast, Input *input)
{
	Lexer lexer;
	Fsm *ebnf_fsm = c_new(Fsm, 1);

	EventListener ebnf_listener;
	ebnf_listener.target = ast;
	ebnf_listener.handler = ebnf_fsm_ast_handler;

	lexer_init(&lexer, input);
	ebnf_init_fsm(ebnf_fsm);
	ast_init(ast, input);

	Session *session = fsm_start_session(ebnf_fsm);
	session_set_listener(session, ebnf_listener);

	while (!input->eof) {
		lexer_next(&lexer);
		session_match(session, lexer.symbol, lexer.index);
		if(session->current->type == ACTION_TYPE_ERROR) {
			printf("Error parsing grammar at index: %i\n", session->index);
			break;
		}
	}
	ast_done(ast);
}

void ebnf_build_definitions_list(FsmCursor *f_cur, AstCursor *a_cur);

void ebnf_build_expression(FsmCursor *f_cur, AstCursor *a_cur)
{
	AstNode *node = ast_cursor_depth_next(a_cur);
	switch(node->symbol) {
	case L_IDENTIFIER:
		break;
	case L_TERMINAL_STRING:
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
		ebnf_build_single_definition(f_cur, a_cur);
		ast_cursor_pop(a_cur);
	} while(ast_cursor_next_sibling_symbol(a_cur, E_SINGLE_DEFINITION));
}

void ebnf_build_non_terminal_declaration(FsmCursor *f_cur, AstCursor *a_cur)
{
	unsigned char *string;
	int length;

	ast_cursor_depth_next_symbol(a_cur, L_IDENTIFIER);
	ast_cursor_get_string(a_cur, &string, &length);
	fsm_cursor_set(f_cur, string, length);

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

	ast_cursor_dispose(&a_cur);
}
