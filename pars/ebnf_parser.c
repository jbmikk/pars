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
	Frag *frag;
	Frag *e_frag;

    fsm_init(fsm);

	//Expression
    e_frag = fsm_get_frag(fsm, "expression", 10);
    frag_add_context_shift(e_frag, L_IDENTIFIER);
    frag_add_reduce(e_frag, L_CONCATENATE_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_END_GROUP_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	frag_rewind(e_frag);
    frag_add_context_shift(e_frag, L_TERMINAL_STRING);
    frag_add_reduce(e_frag, L_CONCATENATE_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_END_GROUP_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_TERMINATOR_SYMBOL, E_EXPRESSION);
	frag_rewind(e_frag);
    frag_add_context_shift(e_frag, L_START_GROUP_SYMBOL);
	
	//Single Definition
    frag = fsm_get_frag(fsm, "single_definition", 17);
    frag_add_followset(frag, fsm_get_state(fsm, "expression", 10));
    frag_add_context_shift(frag, E_EXPRESSION);
    frag_add_reduce(frag, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);
    frag_add_shift(frag, L_CONCATENATE_SYMBOL);
    frag_add_followset(frag, fsm_get_state(fsm, "single_definition", 17));
    frag_add_shift(frag, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_DEFINITION_SEPARATOR_SYMBOL, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_TERMINATOR_SYMBOL, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_END_GROUP_SYMBOL, E_SINGLE_DEFINITION);

	//Definitions List
    frag = fsm_get_frag(fsm, "definitions_list", 16);
    frag_add_followset(frag, fsm_get_state(fsm, "single_definition", 17));
    frag_add_context_shift(frag, E_SINGLE_DEFINITION);
    frag_add_reduce(frag, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
    frag_add_reduce(frag, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);
    frag_add_shift(frag, L_DEFINITION_SEPARATOR_SYMBOL);
    frag_add_followset(frag, fsm_get_state(fsm, "definitions_list", 16));
    frag_add_shift(frag, E_DEFINITIONS_LIST);
    frag_add_reduce(frag, L_TERMINATOR_SYMBOL, E_DEFINITIONS_LIST);
    frag_add_reduce(frag, L_END_GROUP_SYMBOL, E_DEFINITIONS_LIST);

	//Finish Expression
    frag_add_followset(e_frag, fsm_get_state(fsm, "definitions_list", 16));
    frag_add_shift(e_frag, E_DEFINITIONS_LIST);
    frag_add_shift(e_frag, L_END_GROUP_SYMBOL);
    frag_add_reduce(e_frag, L_CONCATENATE_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_DEFINITION_SEPARATOR_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_END_GROUP_SYMBOL, E_EXPRESSION);
    frag_add_reduce(e_frag, L_TERMINATOR_SYMBOL, E_EXPRESSION);

	//Non Terminal Declaration
    frag = fsm_get_frag(fsm, "non_terminal_declaration", 24);
    frag_add_context_shift(frag, L_IDENTIFIER);
    frag_add_shift(frag, L_DEFINING_SYMBOL);
    frag_add_followset(frag, fsm_get_state(fsm, "definitions_list", 16));
    frag_add_shift(frag, E_DEFINITIONS_LIST);
    frag_add_shift(frag, L_TERMINATOR_SYMBOL);
    frag_add_reduce(frag, L_IDENTIFIER, E_NON_TERMINAL_DECLARATION);
    frag_add_reduce(frag, L_EOF, E_NON_TERMINAL_DECLARATION);
	
	//Syntax
    frag = fsm_get_frag(fsm, "syntax", 6);
    frag_add_followset(frag, fsm_get_state(fsm, "non_terminal_declaration", 24));
    frag_add_context_shift(frag, E_NON_TERMINAL_DECLARATION);
    frag_add_reduce(frag, L_EOF, E_SYNTAX);
    frag_add_followset(frag, fsm_get_state(fsm, "syntax", 6));
    frag_add_shift(frag, E_SYNTAX);
    frag_add_reduce(frag, L_EOF, E_SYNTAX);

    //fsm_set_start(fsm, "definitions_list", 16, E_SINGLE_DEFINITION);
    fsm_set_start(fsm, "syntax", 6, E_SYNTAX);
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
