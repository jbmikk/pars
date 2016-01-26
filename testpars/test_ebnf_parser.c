#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "fsm.h"
#include "ebnf_parser.h"

#define MATCH(S, I) session_match(&(S), I, 0, 0);
#define TEST(S, I) session_test(&(S), I, 0, 0);

typedef struct {
	Fsm fsm;
} Fixture;

EToken *current;
unsigned int token_index;
unsigned int diff;
unsigned int count;

void h(int token)
{
	if (current[token_index++] != token)
		diff++;
	count++;
}

void setup(Fixture *fix, gconstpointer data){
	token_index = 0;
	diff = 0;
	count = 0;
	ebnf_init_fsm(&fix->fsm);
}

void teardown(Fixture *fix, gconstpointer data){
	fsm_dispose(&fix->fsm);
}

void ebnf_start_parsing__identifier(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "expression", 10);
	State *state;

	fsm_cursor_set_start(&cur, "expression", 10, E_EXPRESSION);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_IDENTIFIER);

	state = TEST(session, L_CONCATENATE_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	state = TEST(session, L_DEFINITION_SEPARATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	state = TEST(session, L_TERMINATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);
	MATCH(session, L_TERMINATOR_SYMBOL);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__terminal(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "expression", 10);
	State *state;

	fsm_cursor_set_start(&cur, "expression", 10, E_EXPRESSION);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_TERMINAL_STRING);

	state = TEST(session, L_CONCATENATE_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	state = TEST(session, L_DEFINITION_SEPARATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	state = TEST(session, L_TERMINATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);
	MATCH(session, L_TERMINATOR_SYMBOL);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__concatenate(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "single_definition", 17);
	State *state;

	fsm_cursor_set_start(&cur, "single_definition", 17, E_SINGLE_DEFINITION);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_CONCATENATE_SYMBOL);
	MATCH(session, L_TERMINAL_STRING);

	state = TEST(session, L_DEFINITION_SEPARATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	state = TEST(session, L_TERMINATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	MATCH(session, L_DEFINITION_SEPARATOR_SYMBOL);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__separator(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "definitions_list", 16);
	State *state;

	fsm_cursor_set_start(&cur, "definitions_list", 16, E_DEFINITIONS_LIST);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, L_TERMINAL_STRING);

	state = TEST(session, L_TERMINATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	MATCH(session, L_TERMINATOR_SYMBOL);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__declaration(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "non_terminal_declaration", 24);
	State *state;

	fsm_cursor_set_start(&cur, "non_terminal_declaration", 24, E_NON_TERMINAL_DECLARATION);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_DEFINING_SYMBOL);
	MATCH(session, L_TERMINAL_STRING);
	MATCH(session, L_DEFINITION_SEPARATOR_SYMBOL);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_TERMINATOR_SYMBOL);

	state = TEST(session, L_EOF);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_NON_TERMINAL_DECLARATION);

	MATCH(session, L_EOF);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__group(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "expression", 10);
	State *state;

	fsm_cursor_set_start(&cur, "expression", 10, E_EXPRESSION);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_START_GROUP_SYMBOL);
	MATCH(session, L_TERMINAL_STRING);
	MATCH(session, L_END_GROUP_SYMBOL);

	state = TEST(session, L_TERMINATOR_SYMBOL);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	g_assert(state->reduction == E_EXPRESSION);

	MATCH(session, L_TERMINATOR_SYMBOL);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

void ebnf_start_parsing__syntax(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_define(&cur, "syntax", 6);
	State *state;

	//Already an accepting state, calling it again causes leak.
	//fsm_cursor_set_start(&cur, "syntax", 6, E_SYNTAX);
	Session session;
	session_init(&session, &fix->fsm);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_DEFINING_SYMBOL);
	MATCH(session, L_TERMINAL_STRING);
	MATCH(session, L_TERMINATOR_SYMBOL);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_DEFINING_SYMBOL);
	MATCH(session, L_IDENTIFIER);
	MATCH(session, L_TERMINATOR_SYMBOL);

	state = TEST(session, L_EOF);
	g_assert(state->type == ACTION_TYPE_REDUCE);
	//First reduction only, not recursive
	g_assert(state->reduction == E_NON_TERMINAL_DECLARATION); 

	MATCH(session, L_EOF);
	g_assert(session.current->type == ACTION_TYPE_ACCEPT);
	session_dispose(&session);

	fsm_cursor_dispose(&cur);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__identifier, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__terminal, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__concatenate, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__separator, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__declaration, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__group, teardown);
	g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__syntax, teardown);
	return g_test_run();
}
