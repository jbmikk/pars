#include <stddef.h>
#include <string.h>

#include "ebnf_parser.h"
#include "fsmprocess.h"
#include "test.h"

#define nzs(S) (S), (strlen(S))

typedef struct {

	Token token;
	Token prev_token;

	//Ebnf lexer tests
	SymbolTable table;
	Fsm fsm;
	FsmProcess process;

	//Utf8 lexer tests
	/*
	Input input_utf8_two_byte;
	Lexer lexer_utf8_two_byte;
	Input input_utf8_three_byte;
	Lexer lexer_utf8_three_byte;
	*/
} Fixture;

Fixture fix;

/*
#define I_UTF8_TWO_BYTE "\xC3\xB1" //U+00F1 ñ
#define I_UTF8_THREE_BYTE "\xE0\xBD\xB1" //U+0F71
*/

static void push_token(void *fix, const Token *token)
{
	Fixture *fixture = (Fixture*)fix;
	fixture->prev_token = fixture->token;
	fixture->token = *token;
}

void t_setup(){
	token_init(&fix.token, 0, 0, 0);
	token_init(&fix.prev_token, 0, 0, 0);

	symbol_table_init(&fix.table);

	fsm_init(&fix.fsm, &fix.table);
	ebnf_build_lexer_fsm(&fix.fsm);

	fsm_process_init(&fix.process, &fix.fsm);
	fix.process.handler.target = &fix;
	fix.process.handler.accept = push_token;
	fsm_process_start(&fix.process);
	//Utf8 tests
	/*
	input_set_data(&fix.input_utf8_two_byte, I_UTF8_TWO_BYTE, strlen(I_UTF8_TWO_BYTE));
	lexer_init(&fix.lexer_utf8_two_byte, &fix.input_utf8_two_byte);
	input_set_data(&fix.input_utf8_three_byte, I_UTF8_THREE_BYTE, strlen(I_UTF8_THREE_BYTE));
	lexer_init(&fix.lexer_utf8_three_byte, &fix.input_utf8_three_byte);
	*/
}

void t_teardown(){
	fsm_process_dispose(&fix.process);
	fsm_dispose(&fix.fsm);
	symbol_table_dispose(&fix.table);
}

void lexer_input_next__integer_token(){
	fsm_process_match(&fix.process, &(Token) {0, 1, '1'});
	fsm_process_match(&fix.process, &(Token) {1, 1, '2'});
	fsm_process_match(&fix.process, &(Token) {2, 1, '3'});
	fsm_process_match(&fix.process, &(Token) {3, 1, '4'});
	fsm_process_match(&fix.process, &(Token) {4, 0, ' '});
	int INTEGER = fsm_get_symbol_id(&fix.fsm, nzs("integer"));
	t_assert(fix.token.symbol == INTEGER);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 4);
}

void lexer_input_next__identifier_token(){
	fsm_process_match(&fix.process, &(Token) {0, 1, 'a'});
	fsm_process_match(&fix.process, &(Token) {1, 1, 'n'});
	fsm_process_match(&fix.process, &(Token) {2, 1, 'I'});
	fsm_process_match(&fix.process, &(Token) {3, 1, 'd'});
	fsm_process_match(&fix.process, &(Token) {4, 1, 'e'});
	fsm_process_match(&fix.process, &(Token) {5, 1, 'n'});
	fsm_process_match(&fix.process, &(Token) {6, 1, 't'});
	fsm_process_match(&fix.process, &(Token) {7, 1, 'i'});
	fsm_process_match(&fix.process, &(Token) {8, 1, 'f'});
	fsm_process_match(&fix.process, &(Token) {9, 1, 'i'});
	fsm_process_match(&fix.process, &(Token) {10, 1, 'e'});
	fsm_process_match(&fix.process, &(Token) {11, 1, 'r'});
	fsm_process_match(&fix.process, &(Token) {12, 1, ' '});
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	t_assert(fix.token.symbol == META_IDENTIFIER);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 12);
}

void lexer_input_next__terminal_string_token(){
	fsm_process_match(&fix.process, &(Token) {0, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {1, 1, 's'});
	fsm_process_match(&fix.process, &(Token) {2, 1, 't'});
	fsm_process_match(&fix.process, &(Token) {3, 1, 'r'});
	fsm_process_match(&fix.process, &(Token) {4, 1, 'i'});
	fsm_process_match(&fix.process, &(Token) {5, 1, 'n'});
	fsm_process_match(&fix.process, &(Token) {6, 1, 'g'});
	fsm_process_match(&fix.process, &(Token) {7, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {8, 1, ' '});
	int TERMINAL_STRING = fsm_get_symbol_id(&fix.fsm, nzs("terminal_string"));
	t_assert(fix.token.symbol == TERMINAL_STRING);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 8);
}

void lexer_input_next__skip_white_space(){
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	int DEFINING_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("defining_symbol"));

	fsm_process_match(&fix.process, &(Token) {0, 1, 'o'});
	fsm_process_match(&fix.process, &(Token) {1, 1, 'n'});
	fsm_process_match(&fix.process, &(Token) {2, 1, 'e'});
	fsm_process_match(&fix.process, &(Token) {3, 1, ' '});
	t_assert(fix.token.symbol == META_IDENTIFIER);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 3);

	fsm_process_match(&fix.process, &(Token) {4, 1, '='});
	fsm_process_match(&fix.process, &(Token) {5, 1, ' '});
	t_assert(fix.token.symbol == DEFINING_SYMBOL);
	t_assert(fix.token.index == 4);
	t_assert(fix.token.length == 1);
}

void lexer_input_next__whole_rule(){
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));
	int DEFINING_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("defining_symbol"));
	int TERMINAL_STRING = fsm_get_symbol_id(&fix.fsm, nzs("terminal_string"));
	int CONCATENATE_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("concatenate_symbol"));
	int START_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("start_group_symbol"));
	int DEFINITION_SEPARATOR_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("definition_separator_symbol"));
	int END_GROUP_SYMBOL = fsm_get_symbol_id(&fix.fsm, nzs("end_group_symbol"));

	fsm_process_match(&fix.process, &(Token) {0, 1, 'o'});
	fsm_process_match(&fix.process, &(Token) {1, 1, 'n'});
	fsm_process_match(&fix.process, &(Token) {2, 1, 'e'});
	fsm_process_match(&fix.process, &(Token) {3, 1, ' '});
	t_assert(fix.token.symbol == META_IDENTIFIER);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 3);

	fsm_process_match(&fix.process, &(Token) {4, 1, '='});
	fsm_process_match(&fix.process, &(Token) {5, 1, ' '});
	t_assert(fix.token.symbol == DEFINING_SYMBOL);
	t_assert(fix.token.index == 4);
	t_assert(fix.token.length == 1);

	fsm_process_match(&fix.process, &(Token) {6, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {7, 1, '1'});
	fsm_process_match(&fix.process, &(Token) {8, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {9, 1, ','});
	t_assert(fix.token.symbol == TERMINAL_STRING);
	t_assert(fix.token.index == 6);
	t_assert(fix.token.length == 3);

	fsm_process_match(&fix.process, &(Token) {10, 1, '('});
	t_assert(fix.token.symbol == CONCATENATE_SYMBOL);
	t_assert(fix.token.index == 9);
	t_assert(fix.token.length == 1);

	fsm_process_match(&fix.process, &(Token) {11, 1, '"'});
	t_assert(fix.token.symbol == START_GROUP_SYMBOL);
	t_assert(fix.token.index == 10);
	t_assert(fix.token.length == 1);

	fsm_process_match(&fix.process, &(Token) {12, 1, 'a'});
	fsm_process_match(&fix.process, &(Token) {13, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {14, 1, '|'});

	t_assert(fix.token.symbol == TERMINAL_STRING);
	t_assert(fix.token.index == 11);
	t_assert(fix.token.length == 3);

	fsm_process_match(&fix.process, &(Token) {15, 1, '"'});
	t_assert(fix.token.symbol == DEFINITION_SEPARATOR_SYMBOL);
	t_assert(fix.token.index == 14);
	t_assert(fix.token.length == 1);

	fsm_process_match(&fix.process, &(Token) {16, 1, 'b'});
	fsm_process_match(&fix.process, &(Token) {17, 1, '"'});
	fsm_process_match(&fix.process, &(Token) {18, 1, ')'});
	t_assert(fix.token.symbol == TERMINAL_STRING);
	t_assert(fix.token.index == 15);
	t_assert(fix.token.length == 3);

	fsm_process_match(&fix.process, &(Token) {19, 0, L_EOF});
	t_assert(fix.prev_token.symbol == END_GROUP_SYMBOL);
	t_assert(fix.prev_token.index == 18);
	t_assert(fix.prev_token.length == 1);

	t_assert(fix.token.symbol == L_EOF);
	t_assert(fix.token.index == 19);
	t_assert(fix.token.length == 0);
}

void lexer_input_next__white_token(){
	int WHITE_SPACE = fsm_get_symbol_id(&fix.fsm, nzs("white_space"));
	int META_IDENTIFIER = fsm_get_symbol_id(&fix.fsm, nzs("meta_identifier"));

	fsm_process_match(&fix.process, &(Token) {0, 1, '\n'});
	fsm_process_match(&fix.process, &(Token) {1, 1, '\r'});
	fsm_process_match(&fix.process, &(Token) {2, 1, '\t'});
	fsm_process_match(&fix.process, &(Token) {3, 1, '\f'});
	fsm_process_match(&fix.process, &(Token) {4, 1, ' '});
	fsm_process_match(&fix.process, &(Token) {5, 1, 'i'});
	fsm_process_match(&fix.process, &(Token) {6, 1, 'd'});

	t_assert(fix.token.symbol == WHITE_SPACE);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 5);

	fsm_process_match(&fix.process, &(Token) {7, 1, ' '});
	t_assert(fix.token.symbol == META_IDENTIFIER);
	t_assert(fix.token.index == 5);
	t_assert(fix.token.length == 2);
}

/*
void lexer_input_next__utf8_two_byte(){
	utf8_lexer(&fix.lexer_utf8_two_byte, &fix.token, &fix.token);
	t_assert(fix.token.symbol == 0xF1);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 2);
}

void lexer_input_next__utf8_three_byte(){
	utf8_lexer(&fix.lexer_utf8_three_byte, &fix.token, &fix.token);
	t_assert(fix.token.symbol == 0xF71);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 3);
}
*/

int main(int argc, char** argv){
	t_init();
	t_test(lexer_input_next__integer_token);
	t_test(lexer_input_next__identifier_token);
	t_test(lexer_input_next__terminal_string_token);
	t_test(lexer_input_next__skip_white_space);
	t_test(lexer_input_next__whole_rule);
	t_test(lexer_input_next__white_token);

	/*
	t_test(lexer_input_next__utf8_two_byte);
	t_test(lexer_input_next__utf8_three_byte);
	*/
	return t_done();
}
