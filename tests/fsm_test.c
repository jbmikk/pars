#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "fsm.h"
#include "fsmthread.h"
#include "fsmbuilder.h"
#include "test.h"

#define nzs(S) (S), (strlen(S))


#define MATCH_START_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_START);

#define MATCH_DROP_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_DROP);

#define MATCH_SHIFT_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_SHIFT);

#define MATCH_REDUCE_AT(S, Y, R, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_REDUCE); \
	t_assert(tran.action->reduction == R);

#define MATCH_ACCEPT_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_ACCEPT);

#define MATCH_ACCEPT_AT_WITH(S, Y, R, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_ACCEPT); \
	t_assert(tran.action->reduction == R);

#define MATCH_EMPTY_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_EMPTY);

#define MATCH_BACKTRACK_AT(S, Y, I) \
	tran = fsm_thread_match(&(S), &(struct Token){ (I), 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert((S).transition.backtrack == 1);

#define MATCH_START(S, Y) MATCH_START_AT(S, Y, 0)
#define MATCH_DROP(S, Y) MATCH_DROP_AT(S, Y, 0)
#define MATCH_SHIFT(S, Y) MATCH_SHIFT_AT(S, Y, 0)
#define MATCH_REDUCE(S, Y, R) MATCH_REDUCE_AT(S, Y, R, 0)
#define MATCH_ACCEPT(S, Y) MATCH_ACCEPT_AT(S, Y, 0)
#define MATCH_ACCEPT_WITH(S, Y, R) MATCH_ACCEPT_AT_WITH(S, Y, R, 0)
#define MATCH_EMPTY(S, Y) MATCH_EMPTY_AT(S, Y, 0)
#define MATCH_BACKTRACK(S, Y) MATCH_BACKTRACK_AT(S, Y, 0)

#define TEST_SHIFT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_SHIFT);

#define TEST_ERROR(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->type == ACTION_ERROR);

#define TEST_ACTION(S, Y, A) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	t_assert(tran.action->end_symbol == (A).end_symbol);


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

void fsm_thread_match__shift(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, 'a');
	MATCH_DROP(thread, 'b');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__shift_range(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_terminal_range(&builder, (Range){'a', 'p'});
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	TEST_SHIFT(thread, 'a');
	TEST_SHIFT(thread, 'b');
	TEST_SHIFT(thread, 'o');
	TEST_SHIFT(thread, 'p');
	TEST_ERROR(thread, 'q');

	state_get_transition(
		fsm_get_state(&fix.fsm, nzs(".default")),
		'z'
	);

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__shift_nested_range(){
	FsmBuilder builder;
	Transition tran;
	Action action1, action2;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'p'});
	action1 = *builder.action;

	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'f', 'l'});
	action2 = *builder.action;

	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	TEST_ACTION(thread, 'a', action1);
	TEST_ACTION(thread, 'b', action1);
	TEST_ACTION(thread, 'f', action2);
	TEST_ACTION(thread, 'g', action2);
	TEST_ACTION(thread, 'l', action2);
	TEST_ACTION(thread, 'o', action1);
	TEST_ACTION(thread, 'p', action1);
	TEST_ERROR(thread, 'q');

	state_get_transition(
		fsm_get_state(&fix.fsm, nzs(".default")),
		'z'
	);

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__shift_path_nested_range(){
	FsmBuilder builder;
	Transition tran;
	Action action1, action2;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("name"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal_range(&builder, (Range){'a', 'p'});
	action1 = *builder.action;

	fsm_builder_or(&builder);
	fsm_builder_terminal_range(&builder, (Range){'f', 'l'});
	action2 = *builder.action;

	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	TEST_ACTION(thread, 'a', action1);
	TEST_ACTION(thread, 'b', action1);
	TEST_ACTION(thread, 'f', action2);
	TEST_ACTION(thread, 'g', action2);
	TEST_ACTION(thread, 'l', action2);
	TEST_ACTION(thread, 'o', action1);
	TEST_ACTION(thread, 'p', action1);
	TEST_ERROR(thread, 'q');

	Action action_a = *state_get_transition(
		fsm_get_state(&fix.fsm, nzs(".default")),
		'g'
	);

	Action action_b = *state_get_path_transition(
		fsm_get_state(&fix.fsm, nzs(".default")),
		'g',
		1
	);

	Action *action_c = state_get_path_transition(
		fsm_get_state(&fix.fsm, nzs(".default")),
		'g',
		2
	);

	t_assert(action_a.end_symbol == action2.end_symbol);
	t_assert(action_b.end_symbol == action1.end_symbol);
	t_assert(action_c == NULL);

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__reduce(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("number"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	int number = fsm_get_symbol_id(&fix.fsm, nzs("number"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, '1');
	MATCH_REDUCE(thread, '\0', number);
	MATCH_DROP(thread, number);
	MATCH_ACCEPT(thread, '\0');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__reduce_shift(){
	FsmBuilder builder;
	Transition tran;

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

	int number = fsm_get_symbol_id(&fix.fsm, nzs("number"));
	int sum = fsm_get_symbol_id(&fix.fsm, nzs("sum"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, '1');
	MATCH_REDUCE(thread, '+', number);
	MATCH_SHIFT(thread, number);
	MATCH_DROP(thread, '+');
	MATCH_DROP(thread, '2');
	MATCH_REDUCE(thread, '\0', sum);
	MATCH_DROP(thread, sum);
	MATCH_ACCEPT(thread, '\0');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__reduce_handler(){
	FsmBuilder builder;
	Transition tran;

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

	int number = fsm_get_symbol_id(&fix.fsm, nzs("number"));
	int word = fsm_get_symbol_id(&fix.fsm, nzs("word"));
	int sum = fsm_get_symbol_id(&fix.fsm, nzs("sum"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT_AT(thread, '1', 0);
	MATCH_REDUCE_AT(thread, '+', number, 1);
	MATCH_SHIFT_AT(thread, number, 0);
	MATCH_DROP_AT(thread, '+', 1);

	MATCH_SHIFT_AT(thread, 'w', 2);
	MATCH_DROP_AT(thread, 'o', 3);
	MATCH_DROP_AT(thread, 'r', 4);
	MATCH_DROP_AT(thread, 'd', 5);
	MATCH_REDUCE_AT(thread, '\0', word, 6);
	MATCH_DROP_AT(thread, word, 2);
	MATCH_REDUCE_AT(thread, '\0', sum, 6);
	MATCH_DROP_AT(thread, sum, 0);
	MATCH_ACCEPT_AT(thread, '\0', 6);

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__first_set_collision(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("A"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '2');
	fsm_builder_terminal(&builder, '3');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("B"));
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '3');
	fsm_builder_terminal(&builder, '4');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("sequence"));
	fsm_builder_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("A"));
	fsm_builder_or(&builder);
	fsm_builder_nonterminal(&builder,  nzs("B"));
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	int a = fsm_get_symbol_id(&fix.fsm, nzs("A"));
	//int b = fsm_get_symbol_id(&fix.fsm, nzs("B"));
	int sequence = fsm_get_symbol_id(&fix.fsm, nzs("sequence"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, '1');
	MATCH_DROP(thread, '2');
	MATCH_DROP(thread, '3');
	MATCH_REDUCE(thread, '\0', a);
	MATCH_SHIFT(thread, a);
	MATCH_REDUCE(thread, '\0', sequence);
	MATCH_DROP(thread, sequence);
	MATCH_ACCEPT(thread, '\0');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__repetition(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("nonZeroDigit"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal(&builder, '1');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '2');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '3');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '4');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '5');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '6');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '7');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '8');
	fsm_builder_or(&builder);
	fsm_builder_terminal(&builder, '9');
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("digit"));
	fsm_builder_group_start(&builder);
	fsm_builder_terminal(&builder, '0');
	fsm_builder_or(&builder);
	fsm_builder_nonterminal(&builder,  nzs("nonZeroDigit"));
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("integer"));
	fsm_builder_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("nonZeroDigit"));
	fsm_builder_loop_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("digit"));
	fsm_builder_loop_group_end(&builder);
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	int nonZeroDigit = fsm_get_symbol_id(&fix.fsm, nzs("nonZeroDigit"));
	int digit = fsm_get_symbol_id(&fix.fsm, nzs("digit"));
	int integer = fsm_get_symbol_id(&fix.fsm, nzs("integer"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, '1');
	MATCH_REDUCE(thread, '2', nonZeroDigit);
	MATCH_SHIFT(thread, nonZeroDigit);

	MATCH_SHIFT(thread, '2');
	MATCH_REDUCE(thread, '3', nonZeroDigit);
	MATCH_SHIFT(thread, nonZeroDigit);
	MATCH_REDUCE(thread, '3', digit);
	MATCH_DROP(thread, digit);

	MATCH_SHIFT(thread, '3');
	MATCH_REDUCE(thread, '\0', nonZeroDigit);
	MATCH_SHIFT(thread, nonZeroDigit);
	MATCH_REDUCE(thread, '\0', digit);
	MATCH_DROP(thread, digit);

	MATCH_EMPTY(thread, '\0');
	MATCH_REDUCE(thread, '\0', integer);
	MATCH_DROP(thread, integer);
	MATCH_ACCEPT(thread, '\0');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__any(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_set_mode(&builder, nzs(".default"));

	fsm_builder_define(&builder, nzs("A"));
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_any(&builder);
	fsm_builder_terminal(&builder, 'c');
	fsm_builder_end(&builder);

	fsm_builder_lexer_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	int a = fsm_get_symbol_id(&fix.fsm, nzs("A"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_START(thread, 'a');
	MATCH_DROP(thread, 'x');
	MATCH_DROP(thread, 'y');
	MATCH_DROP(thread, 'z');
	MATCH_DROP(thread, 'c');
	MATCH_ACCEPT_WITH(thread, '\0', a);

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__simple_backtrack(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	fsm_builder_define(&builder, nzs("A"));
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_group_start(&builder);
	// First it should match this one.
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '2');
	fsm_builder_or(&builder);
	// Then it should backtrack and match this one.
	fsm_builder_any(&builder);
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '3');
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	int a = fsm_get_symbol_id(&fix.fsm, nzs("A"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, 'b');
	MATCH_DROP(thread, 'a');
	MATCH_DROP(thread, '1');
	MATCH_BACKTRACK(thread, '3');
	MATCH_DROP(thread, 'a');
	MATCH_DROP(thread, '1');
	MATCH_DROP(thread, '3');
	MATCH_REDUCE(thread, '\0', a);
	MATCH_DROP(thread, a);
	MATCH_ACCEPT(thread, '\0');

	fsm_thread_dispose(&thread);
}

void fsm_thread_match__backtrack_with_shift(){
	FsmBuilder builder;
	Transition tran;

	fsm_builder_init(&builder, &fix.fsm);

	// First it should match this one.
	fsm_builder_define(&builder, nzs("A"));
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '2');
	fsm_builder_end(&builder);

	// Then it should backtrack and match this one.
	fsm_builder_define(&builder, nzs("B"));
	fsm_builder_any(&builder);
	fsm_builder_terminal(&builder, '1');
	fsm_builder_terminal(&builder, '3');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("sequence"));
	fsm_builder_group_start(&builder);
	fsm_builder_nonterminal(&builder,  nzs("A"));
	fsm_builder_or(&builder);
	fsm_builder_nonterminal(&builder,  nzs("B"));
	fsm_builder_group_end(&builder);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, '\0');

	fsm_builder_dispose(&builder);

	//int a = fsm_get_symbol_id(&fix.fsm, nzs("A"));
	int b = fsm_get_symbol_id(&fix.fsm, nzs("B"));
	int sequence = fsm_get_symbol_id(&fix.fsm, nzs("sequence"));

	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	// Shift and backtracking happen both at the beginning. They both get 
	// pushed to the stack at the same time. The shift must be backtracked
	// so that it happens only once.
	MATCH_SHIFT(thread, 'a');
	MATCH_DROP(thread, '1');
	MATCH_BACKTRACK(thread, '3');
	MATCH_SHIFT(thread, 'a');
	MATCH_DROP(thread, '1');
	MATCH_DROP(thread, '3');
	MATCH_REDUCE(thread, '\0', b);
	MATCH_SHIFT(thread, b);
	MATCH_REDUCE(thread, '\0', sequence);
	MATCH_DROP(thread, sequence);
	MATCH_ACCEPT(thread, '\0');

	t_assert(fsm_thread_stack_is_empty(&thread));

	fsm_thread_dispose(&thread);
}

int main(int argc, char** argv){
	t_init();
	t_test(fsm_builder_define__single_get);
	t_test(fsm_builder_define__two_gets);
	t_test(fsm_thread_match__shift);
	t_test(fsm_thread_match__shift_range);
	t_test(fsm_thread_match__shift_nested_range);
	t_test(fsm_thread_match__shift_path_nested_range);
	t_test(fsm_thread_match__reduce);
	t_test(fsm_thread_match__reduce_shift);
	t_test(fsm_thread_match__reduce_handler);
	t_test(fsm_thread_match__first_set_collision);
	t_test(fsm_thread_match__repetition);
	t_test(fsm_thread_match__any);
	t_test(fsm_thread_match__simple_backtrack);
	t_test(fsm_thread_match__backtrack_with_shift);
	return t_done();
}

