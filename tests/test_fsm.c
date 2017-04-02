#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "fsm.h"
#include "session.h"
#include "fsmbuilder.h"
#include "test.h"

#define nzs(S) (S), (strlen(S))

#define MATCH(S, Y) session_match(&(S), &(struct _Token){ 0, 0, (Y)});
#define MATCH_AT(S, Y, I) session_match(&(S), &(struct _Token){ (I), 0, (Y)});
#define TEST(S, Y) session_test(&(S), &(struct _Token){ 0, 0, (Y)});

typedef struct {
	SymbolTable table;
	Fsm fsm;
} Fixture;

Token token;
Fixture fix;

void t_setup(){
	symbol_table_init(&fix.table);
	fsm_init(&fix.fsm, &fix.table);
}

void t_teardown(){
	fsm_dispose(&fix.fsm);
	symbol_table_dispose(&fix.table);
}

void fsm_builder_define__single_get(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);
	fsm_builder_define(&builder, nzs("name"));
	t_assert(builder.state != NULL);
	fsm_builder_dispose(&builder);
}

void fsm_builder_define__two_gets(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("rule1"));
	State *state1 = builder.state;
	fsm_builder_define(&builder, nzs("rule2"));
	State *state2 = builder.state;
	fsm_builder_define(&builder, nzs("rule1"));
	State *state3 = builder.state;
	fsm_builder_dispose(&builder);

	t_assert(state1 != NULL);
	t_assert(state2 != NULL);
	t_assert(state3 != NULL);
	t_assert(state1 == state3);
}

void session_match__shift(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	MATCH(session, 'a');
	t_assert(session.last_action->type == ACTION_SHIFT);
	MATCH(session, 'b');
	t_assert(session.last_action->type == ACTION_DROP);
	session_dispose(&session);
}

void session_match__shift_range(){
	FsmBuilder builder;
	Action *action;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_terminal_range(&builder, 'a', 'p');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	action = TEST(session, 'a');
	t_assert(action->type == ACTION_SHIFT);
	action = TEST(session, 'b');
	t_assert(action->type == ACTION_SHIFT);
	action = TEST(session, 'o');
	t_assert(action->type == ACTION_SHIFT);
	action = TEST(session, 'p');
	t_assert(action->type == ACTION_SHIFT);
	action = TEST(session, 'q');
	t_assert(action->type == ACTION_ERROR);

	action = state_get_transition(fix.fsm.start, 'z');
	session_dispose(&session);
}

void session_match__reduce(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("number"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	MATCH(session, '1');
	MATCH(session, '\0');
	t_assert(session.last_action->type == ACTION_ACCEPT);
	session_dispose(&session);
}

void session_match__reduce_shift(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("number"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, "sum", 3);
	fsm_builder_nonterminal(&builder,  nzs("number"));
	fsm_builder_terminal(&builder, '+');
	fsm_builder_terminal(&builder, '2');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	Session session;
	session_init(&session, &fix.fsm, NULL_HANDLER);
	MATCH(session, '1');
	MATCH(session, '+');
	MATCH(session, '2');
	MATCH(session, '\0');
	t_assert(session.last_action->type == ACTION_ACCEPT);
	session_dispose(&session);
}

void reduce_handler(void *target, Token *t)
{
	token = *t;
}

void session_match__reduce_handler(){
	FsmBuilder builder;
	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("number"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("word"));
	fsm_builder_terminal(&builder, 'w');
	fsm_builder_terminal(&builder, 'o');
	fsm_builder_terminal(&builder, 'r');
	fsm_builder_terminal(&builder, 'd');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("sum"));
	fsm_builder_nonterminal(&builder,  nzs("number"));
	fsm_builder_terminal(&builder, '+');
	fsm_builder_nonterminal(&builder,  nzs("word"));
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	Session session;
	FsmHandler handler;
	handler.shift = NULL;
	handler.reduce = reduce_handler;
	handler.accept = NULL;
	handler.target = NULL;
	session_init(&session, &fix.fsm, handler);
	MATCH_AT(session, '1', 0);
	MATCH_AT(session, '+', 1);
	t_assert(token.symbol == fsm_get_symbol(&fix.fsm, nzs("number")));
	t_assert(token.index == 0);
	t_assert(token.length == 1);
	t_assert(session.index == 1);
	MATCH_AT(session, 'w', 2);
	MATCH_AT(session, 'o', 3);
	MATCH_AT(session, 'r', 4);
	MATCH_AT(session, 'd', 5);
	MATCH_AT(session, '\0', 6);
	t_assert(token.symbol == fsm_get_symbol(&fix.fsm, nzs("sum")));
	t_assert(token.index == 0);
	t_assert(token.length == 6);
	t_assert(session.last_action->type == ACTION_ACCEPT);
	session_dispose(&session);
}

int main(int argc, char** argv){
	t_init();
	t_test(fsm_builder_define__single_get);
	t_test(fsm_builder_define__two_gets);
	t_test(session_match__shift);
	t_test(session_match__shift_range);
	t_test(session_match__reduce);
	t_test(session_match__reduce_shift);
	t_test(session_match__reduce_handler);
	return t_done();
}

