#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "ebnf_parser.h"
#include "test.h"

#define MATCH(S, Y) session_match(&(S), &(struct _Token){ 0, 0, (Y)});
#define TEST(S, Y) session_test(&(S), &(struct _Token){ 0, 0, (Y)});

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
	ebnf_init_lexer_fsm(&fix.lexer_fsm);

	fsm_init(&fix.fsm, &fix.table);
	ebnf_init_fsm(&fix.fsm);

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

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, fix.META_IDENTIFIER);

	action = TEST(session, fix.CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(session, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.TERMINATOR_SYMBOL);
	session_dispose(&session);
}

void ebnf_start_parsing__terminal(){

	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, fix.TERMINAL_STRING);

	action = TEST(session, fix.CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(session, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__concatenate(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("single_definition"));
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.CONCATENATE_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);

	action = TEST(session, fix.DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.DEFINITION_SEPARATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__separator(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("definitions_list"));
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntactic_term(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_term"));
	MATCH(session, fix.TERMINAL_STRING);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	MATCH(session, fix.EXCEPT_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntax_rule(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntax_rule"));
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.DEFINING_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);
	MATCH(session, fix.DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.TERMINATOR_SYMBOL);

	action = TEST(session, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTAX_RULE);

	action = MATCH(session, L_EOF);
	t_assert(action->type == ACTION_ACCEPT);

	session_dispose(&session);
}

void ebnf_start_parsing__group(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, fix.START_GROUP_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);
	MATCH(session, fix.END_GROUP_SYMBOL);

	action = TEST(session, fix.TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == fix.SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, fix.TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntax(){
	Action *action;

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.DEFINING_SYMBOL);
	MATCH(session, fix.TERMINAL_STRING);
	MATCH(session, fix.TERMINATOR_SYMBOL);
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.DEFINING_SYMBOL);
	MATCH(session, fix.META_IDENTIFIER);
	MATCH(session, fix.TERMINATOR_SYMBOL);

	action = TEST(session, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	//First reduction only, not recursive
	t_assert(action->reduction == fix.SYNTAX_RULE); 

	action = MATCH(session, L_EOF);
	t_assert(action->type == ACTION_ACCEPT);
	session_dispose(&session);
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
