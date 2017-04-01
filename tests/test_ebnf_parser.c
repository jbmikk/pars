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
	fsm_init(&fix.fsm, &fix.table);
	ebnf_init_fsm(&fix.fsm);
}

void t_teardown(){
	fsm_dispose(&fix.fsm);
	symbol_table_dispose(&fix.table);
}

void ebnf_start_parsing__identifier(){

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, E_META_IDENTIFIER);

	action = TEST(session, E_CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_TERMINATOR_SYMBOL);
	session_dispose(&session);
}

void ebnf_start_parsing__terminal(){

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_CONCATENATE_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__concatenate(){
	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("single_definition"));
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_CONCATENATE_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_DEFINITION_SEPARATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__separator(){
	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("definitions_list"));
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntactic_term(){
	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_term"));
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_EXCEPT_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntax_rule(){
	Action *action;
	int E_SYNTAX_RULE = fsm_get_symbol(&fix.fsm, nzs("syntax_rule"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntax_rule"));
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_DEFINING_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);
	MATCH(session, E_DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_TERMINATOR_SYMBOL);

	action = TEST(session, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTAX_RULE);

	MATCH(session, L_EOF);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
}

void ebnf_start_parsing__group(){
	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	session.current = fsm_get_state(&fix.fsm, nzs("syntactic_primary"));
	MATCH(session, E_START_GROUP_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);
	MATCH(session, E_END_GROUP_SYMBOL);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	//TODO: mockup shift before reduction
	//MATCH(session, E_TERMINATOR_SYMBOL);

	session_dispose(&session);
}

void ebnf_start_parsing__syntax(){
	Action *action;

	int E_SYNTAX_RULE = fsm_get_symbol(&fix.fsm, nzs("syntax_rule"));
	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_DEFINING_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);
	MATCH(session, E_TERMINATOR_SYMBOL);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_DEFINING_SYMBOL);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_TERMINATOR_SYMBOL);

	action = TEST(session, L_EOF);
	t_assert(action->type == ACTION_REDUCE);
	//First reduction only, not recursive
	t_assert(action->reduction == E_SYNTAX_RULE); 

	MATCH(session, L_EOF);
	t_assert(session.last_action->type == ACTION_ACCEPT);
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
