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
	FsmCursor e_cur;

	fsm_init(fsm);

	//Expression
	fsm_cursor_init(&e_cur, fsm);
	fsm_cursor_set(&e_cur, "expression", 10);
	fsm_cursor_add_context_shift(&e_cur, L_IDENTIFIER);
	fsm_cursor_add_reduce(&e_cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_rewind(&e_cur);
	fsm_cursor_add_context_shift(&e_cur, L_TERMINAL_STRING);
	fsm_cursor_add_reduce(&e_cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_rewind(&e_cur);
	fsm_cursor_add_context_shift(&e_cur, L_START_GROUP_SYMBOL);

	//Single Definition
	fsm_cursor_init(&cur, fsm);
	fsm_cursor_set(&cur, "single_definition", 17);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "expression", 10));
	fsm_cursor_add_context_shift(&cur, E_EXPRESSION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_shift(&cur, L_CONCATENATE_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "single_definition", 17));
	fsm_cursor_add_shift(&cur, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);

	//Definitions List
	fsm_cursor_init(&cur, fsm);
	fsm_cursor_set(&cur, "definitions_list", 16);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "single_definition", 17));
	fsm_cursor_add_context_shift(&cur, E_SINGLE_DEFINITION);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_DEFINITION_SEPARATOR_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "definitions_list", 16));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_reduce(&cur, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
	fsm_cursor_add_reduce(&cur, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);

	//Finish Expression
	fsm_cursor_add_followset(&e_cur, fsm_get_state(fsm, "definitions_list", 16));
	fsm_cursor_add_shift(&e_cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&e_cur, L_END_GROUP_SYMBOL);
	fsm_cursor_add_reduce(&e_cur, L_CONCATENATE_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_END_GROUP_SYMBOL, E_EXPRESSION);
	fsm_cursor_add_reduce(&e_cur, L_TERMINATOR_SYMBOL, E_EXPRESSION);

	//Non Terminal Declaration
	fsm_cursor_init(&cur, fsm);
	fsm_cursor_set(&cur, "non_terminal_declaration", 24);
	fsm_cursor_add_context_shift(&cur, L_IDENTIFIER);
	fsm_cursor_add_shift(&cur, L_DEFINING_SYMBOL);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "definitions_list", 16));
	fsm_cursor_add_shift(&cur, E_DEFINITIONS_LIST);
	fsm_cursor_add_shift(&cur, L_TERMINATOR_SYMBOL);
	fsm_cursor_add_reduce(&cur, L_IDENTIFIER, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_reduce(&cur, L_EOF, E_NON_TERMINAL_DECLARATION);

	//Syntax
	fsm_cursor_init(&cur, fsm);
	fsm_cursor_set(&cur, "syntax", 6);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "non_terminal_declaration", 24));
	fsm_cursor_add_context_shift(&cur, E_NON_TERMINAL_DECLARATION);
	fsm_cursor_add_reduce(&cur, L_EOF, E_SYNTAX);
	fsm_cursor_add_followset(&cur, fsm_get_state(fsm, "syntax", 6));
	fsm_cursor_add_shift(&cur, E_SYNTAX);
	fsm_cursor_add_reduce(&cur, L_EOF, E_SYNTAX);

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

void ebnf_ast_to_fsm(Fsm *fsm, Ast *ast)
{
	AstCursor cursor;
	AstNode *node;

	ast_cursor_init(&cursor, ast);
	node = ast_cursor_depth_next(&cursor);
	ast_cursor_dispose(&cursor);
}
