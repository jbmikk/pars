#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "fsm.h"
#include "session.h"
#include "fsmcursor.h"
#include "test.h"

#define MATCH(S, Y) session_match(&(S), Y, 0, 0);
#define MATCH_AT(S, Y, I) session_match(&(S), Y, I, 0);

typedef struct {
	SymbolTable table;
	Fsm fsm;
} Fixture;

typedef struct {
	int symbol;
	unsigned int index;
	unsigned int length;
} Token;

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

void fsm_cursor_define__single_get(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);
	fsm_cursor_define(&cur, nzs("name"));
	t_assert(cur.current != NULL);
	fsm_cursor_dispose(&cur);
}

void fsm_cursor_define__two_gets(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);

	fsm_cursor_define(&cur, nzs("rule1"));
	Action *action1 = cur.current;
	fsm_cursor_define(&cur, nzs("rule2"));
	Action *action2 = cur.current;
	fsm_cursor_define(&cur, nzs("rule1"));
	Action *action3 = cur.current;
	fsm_cursor_dispose(&cur);

	t_assert(action1 != NULL);
	t_assert(action2 != NULL);
	t_assert(action3 != NULL);
	t_assert(action1 == action3);
}

void session_match__shift(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);

	fsm_cursor_define(&cur, nzs("name"));
	fsm_cursor_terminal(&cur, 'a');
	fsm_cursor_terminal(&cur, 'b');
	fsm_cursor_end(&cur);

	fsm_cursor_done(&cur, '\0');

	fsm_cursor_dispose(&cur);

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, 'a');
	t_assert(session.last_action->type == ACTION_CONTEXT_SHIFT);
	MATCH(session, 'b');
	t_assert(session.last_action->type == ACTION_SHIFT);
	session_dispose(&session);
}

void session_match__reduce(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);

	fsm_cursor_define(&cur, nzs("number"));
	fsm_cursor_terminal(&cur, '1');
	fsm_cursor_end(&cur);

	fsm_cursor_done(&cur, '\0');

	fsm_cursor_dispose(&cur);

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, '1');
	MATCH(session, '\0');
	t_assert(session.last_action->type == ACTION_ACCEPT);
	session_dispose(&session);
}

void session_match__reduce_shift(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);

	fsm_cursor_define(&cur, nzs("number"));
	fsm_cursor_terminal(&cur, '1');
	fsm_cursor_end(&cur);

	fsm_cursor_define(&cur, "sum", 3);
	fsm_cursor_nonterminal(&cur,  nzs("number"));
	fsm_cursor_terminal(&cur, '+');
	fsm_cursor_terminal(&cur, '2');
	fsm_cursor_end(&cur);

	fsm_cursor_done(&cur, '\0');

	fsm_cursor_dispose(&cur);

	Session session;
	session_init(&session, &fix.fsm);
	MATCH(session, '1');
	MATCH(session, '+');
	MATCH(session, '2');
	MATCH(session, '\0');
	t_assert(session.last_action->type == ACTION_ACCEPT);
	session_dispose(&session);
}

void reduce_handler(void *target, unsigned int index, unsigned int length, int symbol)
{
	token.index = index;
	token.length = length;
	token.symbol = symbol;
}

void session_match__reduce_handler(){
	FsmCursor cur;
	fsm_cursor_init(&cur, &fix.fsm);

	fsm_cursor_define(&cur, nzs("number"));
	fsm_cursor_terminal(&cur, '1');
	fsm_cursor_end(&cur);

	fsm_cursor_define(&cur, nzs("word"));
	fsm_cursor_terminal(&cur, 'w');
	fsm_cursor_terminal(&cur, 'o');
	fsm_cursor_terminal(&cur, 'r');
	fsm_cursor_terminal(&cur, 'd');
	fsm_cursor_end(&cur);

	fsm_cursor_define(&cur, nzs("sum"));
	fsm_cursor_nonterminal(&cur,  nzs("number"));
	fsm_cursor_terminal(&cur, '+');
	fsm_cursor_nonterminal(&cur,  nzs("word"));
	fsm_cursor_end(&cur);

	fsm_cursor_done(&cur, '\0');

	fsm_cursor_dispose(&cur);

	Session session;
	session_init(&session, &fix.fsm);
	FsmHandler handler;
	handler.context_shift = NULL;
	handler.reduce = reduce_handler;
	session_set_handler(&session, handler, NULL);
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
	t_test(fsm_cursor_define__single_get);
	t_test(fsm_cursor_define__two_gets);
	t_test(session_match__shift);
	t_test(session_match__reduce);
	t_test(session_match__reduce_shift);
	t_test(session_match__reduce_handler);
	return t_done();
}

