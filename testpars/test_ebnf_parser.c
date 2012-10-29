#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "fsm.h"
#include "ebnf_parser.h"

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
	init_ebnf_parser(&fix->fsm);
}

void teardown(Fixture *fix, gconstpointer data){
}

void ebnf_start_parsing__identifier(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "expression", 10);
	State *state;

    fsm_set_start(&fix->fsm, "expression", 10, E_EXPRESSION);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_IDENTIFIER);

    state = session_test(session, L_CONCATENATE_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    state = session_test(session, L_DEFINITION_SEPARATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    state = session_test(session, L_TERMINATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);
    session_match(session, L_TERMINATOR_SYMBOL);
}

void ebnf_start_parsing__terminal(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "expression", 10);
	State *state;

    fsm_set_start(&fix->fsm, "expression", 10, E_EXPRESSION);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_TERMINAL_STRING);

    state = session_test(session, L_CONCATENATE_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    state = session_test(session, L_DEFINITION_SEPARATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    state = session_test(session, L_TERMINATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);
    session_match(session, L_TERMINATOR_SYMBOL);
}

void ebnf_start_parsing__concatenate(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "single_definition", 17);
	State *state;

    fsm_set_start(&fix->fsm, "single_definition", 17, E_SINGLE_DEFINITION);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_CONCATENATE_SYMBOL);
    session_match(session, L_TERMINAL_STRING);

    state = session_test(session, L_DEFINITION_SEPARATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    state = session_test(session, L_TERMINATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    session_match(session, L_DEFINITION_SEPARATOR_SYMBOL);
}

void ebnf_start_parsing__separator(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "definitions_list", 16);
	State *state;

    fsm_set_start(&fix->fsm, "definitions_list", 16, E_DEFINITIONS_LIST);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_DEFINITION_SEPARATOR_SYMBOL);
    session_match(session, L_TERMINAL_STRING);

    state = session_test(session, L_TERMINATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    session_match(session, L_TERMINATOR_SYMBOL);
}

void ebnf_start_parsing__declaration(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "non_terminal_declaration", 24);
	State *state;

    fsm_set_start(&fix->fsm, "non_terminal_declaration", 24, E_NON_TERMINAL_DECLARATION);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_DEFINING_SYMBOL);
    session_match(session, L_TERMINAL_STRING);
    session_match(session, L_DEFINITION_SEPARATOR_SYMBOL);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_TERMINATOR_SYMBOL);

    state = session_test(session, L_EOF);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_NON_TERMINAL_DECLARATION);

    session_match(session, L_EOF);
}

void ebnf_start_parsing__group(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "expression", 10);
	State *state;

    fsm_set_start(&fix->fsm, "expression", 10, E_EXPRESSION);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_START_GROUP_SYMBOL);
    session_match(session, L_TERMINAL_STRING);
    session_match(session, L_END_GROUP_SYMBOL);

    state = session_test(session, L_TERMINATOR_SYMBOL);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_EXPRESSION);

    session_match(session, L_TERMINATOR_SYMBOL);
}

void ebnf_start_parsing__syntax(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "syntax", 6);
	State *state;

    fsm_set_start(&fix->fsm, "syntax", 6, E_SYNTAX);
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_DEFINING_SYMBOL);
    session_match(session, L_TERMINAL_STRING);
    session_match(session, L_TERMINATOR_SYMBOL);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_DEFINING_SYMBOL);
    session_match(session, L_IDENTIFIER);
    session_match(session, L_TERMINATOR_SYMBOL);

    state = session_test(session, L_EOF);
    g_assert(state->type == ACTION_TYPE_REDUCE);
    g_assert(state->reduction == E_NON_TERMINAL_DECLARATION); //OK?

    session_match(session, L_EOF);
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
