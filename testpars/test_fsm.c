#include <stddef.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "fsm.h"

#define MATCH(S, Y) session_match(S, Y, 0);
#define MATCH_AT(S, Y, I) session_match(S, Y, I);

typedef struct {
    Fsm fsm;
} Fixture;

FsmArgs reduction;

void setup(Fixture *fix, gconstpointer data){
    fsm_init(&fix->fsm);
}

void teardown(Fixture *fix, gconstpointer data){
    fsm_dispose(&fix->fsm);
}

void fsm_get_frag__single_get(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "name", 4);
    g_assert(frag != NULL);
}

void fsm_get_frag__two_gets(Fixture *fix, gconstpointer data){
    Frag *frag1 = fsm_get_frag(&fix->fsm, "rule1", 4);
    Frag *frag2 = fsm_get_frag(&fix->fsm, "rule2", 4);
    Frag *frag3 = fsm_get_frag(&fix->fsm, "rule1", 4);
    g_assert(frag1 != NULL);
    g_assert(frag2 != NULL);
    g_assert(frag3 != NULL);
    g_assert(frag1 == frag3);
}

void session_match__shift(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "name", 4);
    frag_add_shift(frag, 'a');
    frag_add_shift(frag, 'b');
    frag_add_context_shift(frag, '.');

    fsm_set_start(&fix->fsm, "name", 4, 'N');

    Session *session = fsm_start_session(&fix->fsm);
	MATCH(session, 'a');
	MATCH(session, 'b');
	MATCH(session, '.');
    g_assert(frag != NULL);
    g_assert(session->current->type == ACTION_TYPE_CONTEXT_SHIFT);
}

void session_match__reduce(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "number", 6);
    frag_add_context_shift(frag, '1');
    frag_add_reduce(frag, '\0', 'N');

    fsm_set_start(&fix->fsm, "number", 6, 'N');
    Session *session = fsm_start_session(&fix->fsm);
	MATCH(session, '1');
	MATCH(session, '\0');
    g_assert(session->current->type == ACTION_TYPE_ACCEPT);
}

void session_match__reduce_shift(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "number", 6);
    frag_add_context_shift(frag, '1');
    frag_add_reduce(frag, '+', 'N');

    frag = fsm_get_frag(&fix->fsm, "sum", 3);
    frag_add_followset(frag, fsm_get_state(&fix->fsm, "number", 6));
    frag_add_context_shift(frag, 'N');
    frag_add_shift(frag, '+');
    frag_add_shift(frag, '2');
    frag_add_reduce(frag, '\0', 'S');

    fsm_set_start(&fix->fsm, "sum", 3, 'S');

    Session *session = fsm_start_session(&fix->fsm);
	MATCH(session, '1');
	MATCH(session, '+');
	MATCH(session, '2');
	MATCH(session, '\0');
    g_assert(session->current->type == ACTION_TYPE_ACCEPT);
}

int reduce_handler(int type, void *target, void *args) {
	if(type == EVENT_REDUCE)
		reduction = *((FsmArgs *)args);
}

void session_match__reduce_handler(Fixture *fix, gconstpointer data){
	Frag *frag = fsm_get_frag(&fix->fsm, "number", 6);
	frag_add_context_shift(frag, '1');
	frag_add_reduce(frag, '+', 'N');

	frag = fsm_get_frag(&fix->fsm, "word", 4);
	frag_add_context_shift(frag, 'w');
	frag_add_shift(frag, 'o');
	frag_add_shift(frag, 'r');
	frag_add_shift(frag, 'd');
	frag_add_reduce(frag, '\0', 'W');

	frag = fsm_get_frag(&fix->fsm, "sum", 3);
	frag_add_followset(frag, fsm_get_state(&fix->fsm, "number", 6));
	frag_add_context_shift(frag, 'N');
	frag_add_shift(frag, '+');
	frag_add_followset(frag, fsm_get_state(&fix->fsm, "word", 4));
	frag_add_shift(frag, 'W');
	frag_add_reduce(frag, '\0', 'S');

	fsm_set_start(&fix->fsm, "sum", 3, 'S');

	Session *session = fsm_start_session(&fix->fsm);
	EventListener listener;
	listener.target = NULL;
	listener.handler = reduce_handler;
	session_set_listener(session, listener);
	MATCH_AT(session, '1', 0);
	MATCH_AT(session, '+', 1);
	g_assert_cmpint(reduction.symbol, ==, 'N');
	g_assert_cmpint(reduction.index, ==, 0);
	g_assert_cmpint(reduction.length, ==, 1);
	g_assert_cmpint(session->index, ==, 1);
	MATCH_AT(session, 'w', 2);
	MATCH_AT(session, 'o', 3);
	MATCH_AT(session, 'r', 4);
	MATCH_AT(session, 'd', 5);
	MATCH_AT(session, '\0', 6);
	g_assert_cmpint(reduction.symbol, ==, 'S');
	g_assert_cmpint(reduction.index, ==, 0);
	g_assert_cmpint(reduction.length, ==, 6);
	g_assert(session->current->type == ACTION_TYPE_ACCEPT);
}

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/FSM/get_frag", Fixture, NULL, setup, fsm_get_frag__single_get, teardown);
    g_test_add("/FSM/get_frag", Fixture, NULL, setup, fsm_get_frag__two_gets, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__shift, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce_shift, teardown);
	g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce_handler, teardown);
    return g_test_run();
}

