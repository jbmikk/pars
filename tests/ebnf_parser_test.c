#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "ebnf_parser.h"
#include "test.h"

#define MATCH(S, Y) fsm_thread_match(&(S), &(struct _Token){ 0, 0, (Y)});
#define TEST(S, Y) fsm_thread_test(&(S), &(struct _Token){ 0, 0, (Y)});

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
	int SYNTAX_RULE;
	int SYNTACTIC_PRIMARY;
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
	fix.SYNTAX_RULE = fsm_get_symbol_id(&fix.fsm, nzs("syntax_rule"));
	fix.SYNTACTIC_PRIMARY = fsm_get_symbol_id(&fix.fsm, nzs("syntactic_primary"));
}

void t_teardown(){
	fsm_dispose(&fix.fsm);
	fsm_dispose(&fix.lexer_fsm);
	symbol_table_dispose(&fix.table);
}

void ebnf_start_parsing__identifier(){

	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(thread, fix.META_IDENTIFIER);

	action = TEST(thread, fix.CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.TERMINATOR_SYMBOL);
	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__terminal(){

	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(thread, fix.TERMINAL_STRING);

	action = TEST(thread, fix.CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.TERMINATOR_SYMBOL);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__concatenate(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("single_definition"));
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.CONCATENATE_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);

	action = TEST(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.DEFINITION_SEPARATOR_SYMBOL);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__separator(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("definitions_list"));
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.TERMINATOR_SYMBOL);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntactic_term(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("syntactic_term"));
	MATCH(thread, fix.TERMINAL_STRING);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	MATCH(thread, fix.EXCEPT_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.TERMINATOR_SYMBOL);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntax_rule(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("syntax_rule"));
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.DEFINING_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);
	MATCH(thread, fix.DEFINITION_SEPARATOR_SYMBOL);
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.TERMINATOR_SYMBOL);

	action = TEST(thread, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTAX_RULE);

	action = MATCH(thread, L_EOF);
	t_assert(action->type == ACTION_ACCEPT);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__group(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	thread.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(thread, fix.START_GROUP_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);
	MATCH(thread, fix.END_GROUP_SYMBOL);

	action = TEST(thread, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(thread, fix.TERMINATOR_SYMBOL);

	fsm_thread_dispose(&thread);
}

void ebnf_start_parsing__syntax(){
	Action *action;

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm);
	fsm_thread_start(&thread);
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.DEFINING_SYMBOL);
	MATCH(thread, fix.TERMINAL_STRING);
	MATCH(thread, fix.TERMINATOR_SYMBOL);
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.DEFINING_SYMBOL);
	MATCH(thread, fix.META_IDENTIFIER);
	MATCH(thread, fix.TERMINATOR_SYMBOL);

	action = TEST(thread, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	//First reduction only, not recursive
	t_assert(action->reduction == fix.SYNTAX_RULE); 

	action = MATCH(thread, L_EOF);
	t_assert(action->type == ACTION_ACCEPT);
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
