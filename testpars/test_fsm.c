#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "fsm.h"

typedef struct {
    Fsm fsm;
} Fixture;

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
    session_match(session, 'a');
    session_match(session, 'b');
    session_match(session, '.');
    g_assert(frag != NULL);
    g_assert(session->current->type == ACTION_TYPE_CONTEXT_SHIFT);
}

void session_match__reduce(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "number", 6);
    frag_add_context_shift(frag, '1');
    frag_add_reduce(frag, '\0', 'N');

    fsm_set_start(&fix->fsm, "number", 6, 'N');
    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, '1');
    session_match(session, '\0');
    g_assert(session->current->type == ACTION_TYPE_ACCEPT);
}

void session_match__reduce_shift(Fixture *fix, gconstpointer data){
    Frag *frag = fsm_get_frag(&fix->fsm, "number", 6);
    frag_add_context_shift(frag, '1');
    frag_add_reduce(frag, '+', 'N');

    frag = fsm_get_frag(&fix->fsm, "sum", 3);
    frag_add_include(frag, fsm_get_state(&fix->fsm, "number", 6));
    frag_add_context_shift(frag, 'N');
    frag_add_shift(frag, '+');
    frag_add_shift(frag, '2');
    frag_add_reduce(frag, '\0', 'S');

    fsm_set_start(&fix->fsm, "sum", 3, 'S');

    Session *session = fsm_start_session(&fix->fsm);
    session_match(session, '1');
    session_match(session, '+');
    session_match(session, '2');
    session_match(session, '\0');
    g_assert(frag != NULL);
}

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/FSM/get_frag", Fixture, NULL, setup, fsm_get_frag__single_get, teardown);
    g_test_add("/FSM/get_frag", Fixture, NULL, setup, fsm_get_frag__two_gets, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__shift, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce, teardown);
    g_test_add("/Session/match", Fixture, NULL, setup, session_match__reduce_shift, teardown);
    return g_test_run();
}

