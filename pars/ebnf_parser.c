#include "ebnf_parser.h"
#include "ast.h"
#include "processor.h"

#include <setjmp.h>

jmp_buf on_error;

void parse_error(LInput *input, unsigned int index)
{
    longjmp(on_error, 1);
}

void match(LInput *input, LToken token)
{
    LToken t;
    t = lexer_input_next(input);
    if(t != token) {
        parse_error(input, lexer_input_get_index(input));
    }
}

void init_ebnf_fsm(Fsm *fsm)
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

int event_handler(int type, void *target, void *args) {
	Processor *proc = (Processor *)target;
	FsmArgs *red = (FsmArgs *)args;

	switch(type) {
	case EVENT_REDUCE:
		ast_close(&proc->ast, red->index, red->length, red->symbol);
		break;
	case EVENT_CONTEXT_SHIFT:
		ast_open(&proc->ast, red->index, red->length);
		break;
	}
}

void init_ebnf_interpreter(Processor *processor, Fsm *fsm)
{
	EventListener listener;
	listener.target = processor;
	listener.handler = event_handler;
	processor_init(processor, fsm, listener);
}
