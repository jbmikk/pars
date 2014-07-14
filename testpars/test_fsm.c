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

void fsm_cursor_set__single_get(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);
	fsm_cursor_set(&cur, "name", 4);
	g_assert(cur.current != NULL);
}

void fsm_cursor_set__two_gets(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_set(&cur, "rule1", 5);
	State *state1 = cur.current;
	fsm_cursor_set(&cur, "rule2", 5);
	State *state2 = cur.current;
	fsm_cursor_set(&cur, "rule1", 5);
	State *state3 = cur.current;

	g_assert(state1 != NULL);
	g_assert(state2 != NULL);
	g_assert(state3 != NULL);
	g_assert(state1 == state3);
}

void session_match__shift(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_set(&cur, "name", 4);
	fsm_cursor_add_shift(&cur, 'a');
	fsm_cursor_add_shift(&cur, 'b');
	fsm_cursor_add_context_shift(&cur, '.');

	fsm_cursor_set_start(&cur, "name", 4, 'N');

	Session *session = fsm_start_session(&fix->fsm);
	MATCH(session, 'a');
	MATCH(session, 'b');
	MATCH(session, '.');
	g_assert(session->current->type == ACTION_TYPE_CONTEXT_SHIFT);
}

void session_match__reduce(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_set(&cur, "number", 6);
	fsm_cursor_add_context_shift(&cur, '1');
	fsm_cursor_add_reduce(&cur, '\0', 'N');

	fsm_cursor_set_start(&cur, "number", 6, 'N');
	Session *session = fsm_start_session(&fix->fsm);
	MATCH(session, '1');
	MATCH(session, '\0');
	g_assert(session->current->type == ACTION_TYPE_ACCEPT);
}

void session_match__reduce_shift(Fixture *fix, gconstpointer data){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_set(&cur, "number", 6);
	fsm_cursor_add_context_shift(&cur, '1');
	fsm_cursor_add_reduce(&cur, '+', 'N');

	fsm_cursor_set(&cur, "sum", 3);
	fsm_cursor_add_followset(&cur, fsm_get_state(&fix->fsm, "number", 6));
	fsm_cursor_add_context_shift(&cur, 'N');
	fsm_cursor_add_shift(&cur, '+');
	fsm_cursor_add_shift(&cur, '2');
	fsm_cursor_add_reduce(&cur, '\0', 'S');

	fsm_cursor_set_start(&cur, "sum", 3, 'S');

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
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix->fsm);

	fsm_cursor_set(&cur, "number", 6);
	fsm_cursor_add_context_shift(&cur, '1');
	fsm_cursor_add_reduce(&cur, '+', 'N');

	fsm_cursor_set(&cur, "word", 4);
	fsm_cursor_add_context_shift(&cur, 'w');
	fsm_cursor_add_shift(&cur, 'o');
	fsm_cursor_add_shift(&cur, 'r');
	fsm_cursor_add_shift(&cur, 'd');
	fsm_cursor_add_reduce(&cur, '\0', 'W');

	fsm_cursor_set(&cur, "sum", 3);
	fsm_cursor_add_followset(&cur, fsm_get_state(&fix->fsm, "number", 6));
	fsm_cursor_add_context_shift(&cur, 'N');
	fsm_cursor_add_shift(&cur, '+');
	fsm_cursor_add_followset(&cur, fsm_get_state(&fix->fsm, "word", 4));
	fsm_cursor_add_shift(&cur, 'W');
	fsm_cursor_add_reduce(&cur, '\0', 'S');

	fsm_cursor_set_start(&cur, "sum", 3, 'S');

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
	g_test_add("/FSM/fsm_cursor", Fixture, NULL, setup, fsm_cursor_set__single_get, teardown);
	g_test_add("/FSM/fsm_cursor", Fixture, NULL, setup, fsm_cursor_set__two_gets, teardown);
	g_test_add("/Session/match", Fixture, NULL, setup, session_match__shift, teardown);
	g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce, teardown);
	g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce_shift, teardown);
	g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce_handler, teardown);
	return g_test_run();
}

