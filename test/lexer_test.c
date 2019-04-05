#include <stddef.h>
#include <string.h>

#include "ebnf_parser.h"
#include "fsmthread.h"
#include "test.h"

#define MATCH_DROP(T, I, L, S) \
	tran = fsm_thread_match(&(T), &(struct Token){(I), (L), (S)}); \
	fsm_thread_apply(&(T), tran); \
	t_assert(tran.action->type == ACTION_DROP);

#define MATCH_SHIFT(T, I, L, S) \
	tran = fsm_thread_match(&(T), &(struct Token){(I), (L), (S)}); \
	fsm_thread_apply(&(T), tran); \
	t_assert(tran.action->type == ACTION_SHIFT);

#define MATCH_REDUCE(T, I, L, S, R) \
	tran = fsm_thread_match(&(T), &(struct Token){(I), (L), (S)}); \
	fsm_thread_apply(&(T), tran); \
	t_assert(tran.action->type == ACTION_REDUCE); \
	t_assert(tran.action->reduction == R);

#define MATCH_START(T, I, L, S) \
	tran = fsm_thread_match(&(T), &(struct Token){(I), (L), (S)}); \
	fsm_thread_apply(&(T), tran); \
	t_assert(tran.action->type == ACTION_START);

#define MATCH_ACCEPT(T, I, L, S, R) \
	tran = fsm_thread_match(&(T), &(struct Token){(I), (L), (S)}); \
	fsm_thread_apply(&(T), tran); \
	t_assert(tran.action->type == ACTION_ACCEPT); \
	t_assert(tran.action->reduction == R);

#define nzs(S) (S), (strlen(S))

typedef struct {

	//Ebnf lexer tests
	SymbolTable table;
	Fsm fsm;
	FsmThread thread;

	//Utf8 lexer tests
	/*
	Source source_utf8_two_byte;
	Lexer lexer_utf8_two_byte;
	Source source_utf8_three_byte;
	Lexer lexer_utf8_three_byte;
	*/
} Fixture;

Fixture fix;

/*
#define I_UTF8_TWO_BYTE "\xC3\xB1" //U+00F1 ñ
#define I_UTF8_THREE_BYTE "\xE0\xBD\xB1" //U+0F71
*/

void t_setup(){
	symbol_table_init(&fix.table);

	fsm_init(&fix.fsm, &fix.table);
	ebnf_build_lexer_fsm(&fix.fsm);

	fsm_thread_init(&fix.thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&fix.thread);
	//Utf8 tests
	/*
	source_set_data(&fix.source_utf8_two_byte, I_UTF8_TWO_BYTE, strlen(I_UTF8_TWO_BYTE));
	lexer_init(&fix.lexer_utf8_two_byte, &fix.source_utf8_two_byte);
	source_set_data(&fix.source_utf8_three_byte, I_UTF8_THREE_BYTE, strlen(I_UTF8_THREE_BYTE));
	lexer_init(&fix.lexer_utf8_three_byte, &fix.source_utf8_three_byte);
	*/
}

void t_teardown(){
	fsm_thread_dispose(&fix.thread);
	fsm_dispose(&fix.fsm);
	symbol_table_dispose(&fix.table);
}

void lexer_source_next__integer_token(){
	Transition tran;
	int INTEGER = fsm_get_symbol_id(&fix.fsm, nzs("integer"));

	MATCH_START(fix.thread, 0, 1, '1');
	MATCH_DROP(fix.thread, 1, 1, '2');
	MATCH_DROP(fix.thread, 2, 1, '3');
	MATCH_DROP(fix.thread, 3, 1, '4');
	MATCH_ACCEPT(fix.thread, 4, 0, '\0', INTEGER);
}

void lexer_source_next__identifier_token(){
	Transition tran;
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));

	MATCH_START(fix.thread, 0, 1, 'a');
	MATCH_DROP(fix.thread, 1, 1, 'n');
	MATCH_DROP(fix.thread, 2, 1, 'I');
	MATCH_DROP(fix.thread, 3, 1, 'd');
	MATCH_DROP(fix.thread, 4, 1, 'e');
	MATCH_DROP(fix.thread, 5, 1, 'n');
	MATCH_DROP(fix.thread, 6, 1, 't');
	MATCH_DROP(fix.thread, 7, 1, 'i');
	MATCH_DROP(fix.thread, 8, 1, 'f');
	MATCH_DROP(fix.thread, 9, 1, 'i');
	MATCH_DROP(fix.thread, 10, 1, 'e');
	MATCH_DROP(fix.thread, 11, 1, 'r');
	MATCH_ACCEPT(fix.thread, 12, 1, '\0', META_IDENTIFIER);
}

void lexer_source_next__terminal_string_token(){
	Transition tran;
	int TERMINAL_STRING = fsm_get_symbol_id(&fix.fsm, nzs("terminal_string"));

	MATCH_START(fix.thread, 0, 1, '"');
	MATCH_DROP(fix.thread, 1, 1, 's');
	MATCH_DROP(fix.thread, 2, 1, 't');
	MATCH_DROP(fix.thread, 3, 1, 'r');
	MATCH_DROP(fix.thread, 4, 1, 'i');
	MATCH_DROP(fix.thread, 5, 1, 'n');
	MATCH_DROP(fix.thread, 6, 1, 'g');
	MATCH_DROP(fix.thread, 7, 1, '"');
	MATCH_ACCEPT(fix.thread, 8, 1, '\0', TERMINAL_STRING);
}

void lexer_source_next__skip_white_space(){
	Transition tran;
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	int DEFINING_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("defining_symbol"));
	int WHITE_SPACE = fsm_get_symbol_id(&fix.fsm, nzs("white_space"));

	MATCH_START(fix.thread, 0, 1, 'o');
	MATCH_DROP(fix.thread, 1, 1, 'n');
	MATCH_DROP(fix.thread, 2, 1, 'e');
	MATCH_ACCEPT(fix.thread, 3, 1, ' ', META_IDENTIFIER);

	MATCH_START(fix.thread, 3, 1, ' ');
	MATCH_ACCEPT(fix.thread, 4, 1, '=', WHITE_SPACE);

	MATCH_START(fix.thread, 4, 1, '=');
	MATCH_ACCEPT(fix.thread, 5, 1, '\0', DEFINING_SYMBOL);
}

void lexer_source_next__whole_rule(){
	Transition tran;
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	int DEFINING_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("defining_symbol"));
	int TERMINAL_STRING = fsm_get_symbol_id(&fix.fsm, nzs("terminal_string"));
	int CONCATENATE_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("concatenate_symbol"));
	int START_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("start_group_symbol"));
	int DEFINITION_SEPARATOR_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("definition_separator_symbol"));
	int END_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("end_group_symbol"));

	MATCH_START(fix.thread, 0, 1, 'o');
	MATCH_DROP(fix.thread, 1, 1, 'n');
	MATCH_DROP(fix.thread, 2, 1, 'e');
	MATCH_ACCEPT(fix.thread, 3, 1, '=', META_IDENTIFIER);

	MATCH_START(fix.thread, 3, 1, '=');
	MATCH_ACCEPT(fix.thread, 4, 1, '"', DEFINING_SYMBOL);

	MATCH_START(fix.thread, 4, 1, '"');
	MATCH_DROP(fix.thread, 5, 1, '1');
	MATCH_DROP(fix.thread, 6, 1, '"');
	MATCH_ACCEPT(fix.thread, 7, 1, ',', TERMINAL_STRING);

	MATCH_START(fix.thread, 7, 1, ',');
	MATCH_ACCEPT(fix.thread, 8, 1, '(', CONCATENATE_SYMBOL);

	MATCH_START(fix.thread, 8, 1, '(');
	MATCH_ACCEPT(fix.thread, 9, 1, '"', START_GROUP_SYMBOL);

	MATCH_START(fix.thread, 9, 1, '"');
	MATCH_DROP(fix.thread, 10, 1, 'a');
	MATCH_DROP(fix.thread, 11, 1, '"');
	MATCH_ACCEPT(fix.thread, 12, 1, '|', TERMINAL_STRING);

	MATCH_START(fix.thread, 12, 1, '|');
	MATCH_ACCEPT(fix.thread, 13, 1, '"', DEFINITION_SEPARATOR_SYMBOL);

	MATCH_START(fix.thread, 13, 1, '"');
	MATCH_DROP(fix.thread, 14, 1, 'b');
	MATCH_DROP(fix.thread, 15, 1, '"');
	MATCH_ACCEPT(fix.thread, 16, 1, ')', TERMINAL_STRING);

	MATCH_START(fix.thread, 16, 1, ')');
	MATCH_ACCEPT(fix.thread, 17, 1, L_EOF, END_GROUP_SYMBOL);

	//MATCH_ACCEPT(fix.thread, 17, 0, L_EOF);
}

void lexer_source_next__white_token(){
	Transition tran;
	int WHITE_SPACE = fsm_get_symbol_id(&fix.fsm, nzs("white_space"));
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));

	MATCH_START(fix.thread, 0, 1, '\n');
	MATCH_DROP(fix.thread, 1, 1, '\r');
	MATCH_DROP(fix.thread, 2, 1, '\t');
	MATCH_DROP(fix.thread, 3, 1, '\f');
	MATCH_DROP(fix.thread, 4, 1, ' ');
	MATCH_ACCEPT(fix.thread, 5, 1, 'i', WHITE_SPACE);


	MATCH_START(fix.thread, 5, 1, 'i');
	MATCH_DROP(fix.thread, 6, 1, 'd');
	MATCH_ACCEPT(fix.thread, 7, 1, ' ', META_IDENTIFIER);
}

/*
void lexer_source_next__utf8_two_byte(){
	utf8_lexer(&fix.lexer_utf8_two_byte, &fix.token, &fix.token);
	t_assert(fix.token.symbol == 0xF1);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 2);
}

void lexer_source_next__utf8_three_byte(){
	utf8_lexer(&fix.lexer_utf8_three_byte, &fix.token, &fix.token);
	t_assert(fix.token.symbol == 0xF71);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 3);
}
*/

int main(int argc, char** argv){
	t_init();
	t_test(lexer_source_next__integer_token);
	t_test(lexer_source_next__identifier_token);
	t_test(lexer_source_next__terminal_string_token);
	t_test(lexer_source_next__skip_white_space);
	t_test(lexer_source_next__whole_rule);
	t_test(lexer_source_next__white_token);

	/*
	t_test(lexer_source_next__utf8_two_byte);
	t_test(lexer_source_next__utf8_three_byte);
	*/
	return t_done();
}
