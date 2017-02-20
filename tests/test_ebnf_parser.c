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
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("syntactic_primary"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
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

	MATCH(session, E_TERMINATOR_SYMBOL);
	t_assert(session.last_action->type != ACTION_ERROR);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__terminal(){
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("syntactic_primary"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
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

	MATCH(session, E_TERMINATOR_SYMBOL);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__concatenate(){
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("single_definition"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_CONCATENATE_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__separator(){
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("definitions_list"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, E_META_IDENTIFIER);
	MATCH(session, E_DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_TERMINATOR_SYMBOL);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__syntactic_term(){
	FsmCursor cur;

	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("syntactic_term"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_EXCEPT_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_TERMINATOR_SYMBOL);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__syntax_rule(){
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("syntax_rule"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTAX_RULE = fsm_get_symbol(&fix.fsm, nzs("syntax_rule"));

	Session session;
	session_init(&session, &fix.fsm);
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
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__group(){
	FsmCursor cur;

	//Fake main nonterminal
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("syntactic_primary"));
	fsm_cursor_done(&cur, '\0');

	Action *action;
	int E_SYNTACTIC_PRIMARY = fsm_get_symbol(&fix.fsm, nzs("syntactic_primary"));

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, E_START_GROUP_SYMBOL);
	MATCH(session, E_TERMINAL_STRING);
	MATCH(session, E_END_GROUP_SYMBOL);

	action = TEST(session, E_TERMINATOR_SYMBOL);
	t_assert(action->type == ACTION_REDUCE);
	t_assert(action->reduction == E_SYNTACTIC_PRIMARY);

	MATCH(session, E_TERMINATOR_SYMBOL);
	t_assert(session.last_action->type == ACTION_ACCEPT);

	session_dispose(&session);
	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__syntax(){
	Action *action;

	int E_SYNTAX_RULE = fsm_get_symbol(&fix.fsm, nzs("syntax_rule"));
	Session session;
	session_init(&session, &fix.fsm);
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
