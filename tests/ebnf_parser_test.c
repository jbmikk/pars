#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "ebnf_parser.h"
#include "fsmthread.h"
#include "test.h"


#define MATCH_DROP(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_DROP);

#define MATCH_SHIFT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_SHIFT);

#define MATCH_REDUCE(S, Y, R) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_REDUCE); \
	t_assert(tran.action->reduction == R);

#define MATCH_ACCEPT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_ACCEPT);


#define nzs(S) (S), (strlen(S))

typedef struct {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;

	int META_IDENTIFIER;
	int INTEGER;
	int EXCEPT_SYMBOL;
	int CONCATENATE_SYMBOL;
	int DEFINITION_SEPARATOR_SYMBOL;
	int REPETITION_SYMBOL;
	int DEFINING_SYMBOL;
	int TERMINATOR_SYMBOL;
	int TERMINAL_STRING;
	int SPECIAL_SEQUENCE;
	int START_GROUP_SYMBOL;
	int END_GROUP_SYMBOL;
	int START_OPTION_SYMBOL;
	int END_OPTION_SYMBOL;
	int START_REPETITION_SYMBOL;
	int END_REPETITION_SYMBOL;
	int START_CHARACTER_SET;
	int END_CHARACTER_SET;
	int SYNTACTIC_PRIMARY;
	int SYNTACTIC_FACTOR;
	int SYNTACTIC_TERM;
	int SYNTACTIC_EXCEPTION;
	int SINGLE_DEFINITION;
	int DEFINITIONS_LIST;
	int SYNTAX_RULE;
	int SYNTAX;
} Fixture;

Fixture fix;

int *current;
unsigned int token_index;
unsigned int diff;
unsigned int count;

void h(int token)
{
	if (current[token_index++] != token)
		diff++;
	count++;
}

void t_setup(){
	token_index = 0;
	diff = 0;
	count = 0;
	symbol_table_init(&fix.table);

	fsm_init(&fix.lexer_fsm, &fix.table);
	ebnf_build_lexer_fsm(&fix.lexer_fsm);

	fsm_init(&fix.fsm, &fix.table);
	ebnf_build_fsm(&fix.fsm);

	fix.TERMINATOR_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("terminator_symbol"));
	fix.META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	fix.INTEGER = fsm_get_symbol_id(&fix.fsm, nzs("integer"));
	fix.EXCEPT_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("except_symbol"));
	fix.CONCATENATE_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("concatenate_symbol"));
	fix.DEFINITION_SEPARATOR_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("definition_separator_symbol"));
	fix.REPETITION_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("repetition_symbol"));
	fix.DEFINING_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("defining_symbol"));
	fix.TERMINATOR_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("terminator_symbol"));
	fix.TERMINAL_STRING = fsm_get_symbol_id(&fix.fsm, nzs("terminal_string"));
	fix.SPECIAL_SEQUENCE = fsm_get_symbol_id(&fix.fsm, nzs("special_sequence"));
	fix.START_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("start_group_symbol"));
	fix.END_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("end_group_symbol"));
	fix.START_OPTION_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("start_option_symbol"));
	fix.END_OPTION_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("end_option_symbol"));
	fix.START_REPETITION_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("start_repetition_symbol"));
	fix.END_REPETITION_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("end_repetition_symbol"));
	fix.START_CHARACTER_SET= fsm_get_symbol_id(&fix.fsm, nzs("start_character_set"));
	fix.END_CHARACTER_SET = fsm_get_symbol_id(&fix.fsm, nzs("end_character_set"));
	fix.SYNTACTIC_PRIMARY = fsm_get_symbol_id(&fix.fsm, nzs("syntactic_primary"));
	fix.SYNTACTIC_FACTOR = fsm_get_symbol_id(&fix.fsm, nzs("syntactic_factor"));
	fix.SYNTACTIC_TERM = fsm_get_symbol_id(&fix.fsm, nzs("syntactic_term"));
	fix.SYNTACTIC_EXCEPTION = fsm_get_symbol_id(&fix.fsm, nzs("syntactic_exception"));
	fix.SINGLE_DEFINITION = fsm_get_symbol_id(&fix.fsm, nzs("single_definition"));
	fix.DEFINITIONS_LIST = fsm_get_symbol_id(&fix.fsm, nzs("definitions_list"));
	fix.SYNTAX_RULE = fsm_get_symbol_id(&fix.fsm, nzs("syntax_rule"));
	fix.SYNTAX = fsm_get_symbol_id(&fix.fsm, nzs("syntax"));
}

void t_teardown(){
	fsm_dispose(&fix.fsm);
	fsm_dispose(&fix.lexer_fsm);
	symbol_table_dispose(&fix.table);
}

void ebnf_start_parsing__identifier(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));

	MATCH_DROP(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.CONCATENATE_SYMBOL, fix.SYNTACTIC_PRIMARY);

	// Test DEFINITION_SEPARATOR_SYMBOL and TERMINATOR_SYMBOL

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__terminal(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));

	MATCH_DROP(thread, fix.TERMINAL_STRING);
	MATCH_REDUCE(thread, fix.CONCATENATE_SYMBOL, fix.SYNTACTIC_PRIMARY);

	// Test DEFINITION_SEPARATOR_SYMBOL and TERMINATOR_SYMBOL

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__concatenate(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("single_definition"));

	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.CONCATENATE_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.CONCATENATE_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.CONCATENATE_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_DROP(thread, fix.SYNTACTIC_TERM);
	MATCH_DROP(thread, fix.CONCATENATE_SYMBOL);
	MATCH_SHIFT(thread, fix.TERMINAL_STRING);

	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_DROP(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__separator(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("definitions_list"));

	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_DROP(thread, fix.SINGLE_DEFINITION);
	MATCH_DROP(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	MATCH_SHIFT(thread, fix.TERMINAL_STRING);

	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_DROP(thread, fix.SINGLE_DEFINITION);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.DEFINITIONS_LIST);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntactic_term(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("syntactic_term"));

	MATCH_SHIFT(thread, fix.TERMINAL_STRING);

	MATCH_REDUCE(thread, fix.EXCEPT_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.EXCEPT_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_DROP(thread, fix.SYNTACTIC_FACTOR);
	MATCH_DROP(thread, fix.EXCEPT_SYMBOL);
	MATCH_SHIFT(thread, fix.TERMINAL_STRING);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_EXCEPTION);
	MATCH_DROP(thread, fix.SYNTACTIC_EXCEPTION);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntax_rule(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("syntax_rule"));

	// Should be SHIFT
	MATCH_DROP(thread, fix.META_IDENTIFIER);
	MATCH_DROP(thread, fix.DEFINING_SYMBOL);
	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.DEFINITION_SEPARATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_SHIFT(thread, fix.SINGLE_DEFINITION);
	MATCH_DROP(thread, fix.DEFINITION_SEPARATOR_SYMBOL);

	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_DROP(thread, fix.SINGLE_DEFINITION);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.TERMINATOR_SYMBOL);

	MATCH_REDUCE(thread, L_EOF, fix.SYNTAX_RULE);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__group(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.transition.state = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH_DROP(thread, fix.START_GROUP_SYMBOL);
	MATCH_SHIFT(thread, fix.TERMINAL_STRING);
	MATCH_REDUCE(thread, fix.END_GROUP_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.END_GROUP_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.END_GROUP_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.END_GROUP_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_SHIFT(thread, fix.SINGLE_DEFINITION);
	MATCH_REDUCE(thread, fix.END_GROUP_SYMBOL, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.END_GROUP_SYMBOL);

	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntax(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_DROP(thread, fix.DEFINING_SYMBOL);
	MATCH_SHIFT(thread, fix.TERMINAL_STRING);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_SHIFT(thread, fix.SINGLE_DEFINITION);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.TERMINATOR_SYMBOL);

	MATCH_REDUCE(thread, fix.META_IDENTIFIER, fix.SYNTAX_RULE);
	MATCH_SHIFT(thread, fix.SYNTAX_RULE);

	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_DROP(thread, fix.DEFINING_SYMBOL);
	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_SHIFT(thread, fix.SYNTACTIC_PRIMARY);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_SHIFT(thread, fix.SYNTACTIC_FACTOR);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_SHIFT(thread, fix.SYNTACTIC_TERM);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_SHIFT(thread, fix.SINGLE_DEFINITION);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.DEFINITIONS_LIST);
	MATCH_DROP(thread, fix.TERMINATOR_SYMBOL);

	MATCH_REDUCE(thread, L_EOF, fix.SYNTAX_RULE);
	MATCH_DROP(thread, fix.SYNTAX_RULE);
	MATCH_REDUCE(thread, L_EOF, fix.SYNTAX);
	MATCH_DROP(thread, fix.SYNTAX);

	MATCH_ACCEPT(thread, L_EOF);

	fsm_thread_dispose(&thread);
}

int main(int argc, char** argv){
	t_init(&argc, &argv, NULL);
	t_test(ebnf_start_parsing__identifier);
	t_test(ebnf_start_parsing__terminal);
	t_test(ebnf_start_parsing__concatenate);
	t_test(ebnf_start_parsing__separator);
	t_test(ebnf_start_parsing__syntactic_term);
	t_test(ebnf_start_parsing__syntax_rule);
	t_test(ebnf_start_parsing__group);
	t_test(ebnf_start_parsing__syntax);
	return t_done();
}
