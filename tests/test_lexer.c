#include <stddef.h>
#include <string.h>

#include "lexer.h"
#include "ebnf_lexer.h"
#include "test.h"

typedef struct {

	Token token;

	//Ebnf lexer tests
	Input input_integer;
	Input input_identifier;
	Input input_terminal_string;
	Input input_rule_one;
	Input input_white;
	Lexer lexer_integer;
	Lexer lexer_identifier;
	Lexer lexer_terminal_string;
	Lexer lexer_rule_one;
	Lexer lexer_white;

	//Utf8 lexer tests
	Input input_utf8_two_byte;
	Lexer lexer_utf8_two_byte;
	Input input_utf8_three_byte;
	Lexer lexer_utf8_three_byte;
} Fixture;

Fixture fix;

#define I_INTEGER "1234"
#define I_IDENTIFIER "anIdentifier"
#define I_TERMINAL_STRING "\"terminalString\""
#define I_RULE_ONE "one = \"1\",(\"a\"|\"b\")"
#define I_WHITE "\n\r\t\f identifier"
#define I_UTF8_TWO_BYTE "\xC3\xB1" //U+00F1 ñ
#define I_UTF8_THREE_BYTE "\xE0\xBD\xB1" //U+0F71

void t_setup(){
	token_init(&fix.token, 0, 0, 0);
	input_init_buffer(&fix.input_integer, I_INTEGER, strlen(I_INTEGER));
	input_init_buffer(&fix.input_identifier, I_IDENTIFIER, strlen(I_IDENTIFIER));
	input_init_buffer(&fix.input_terminal_string, I_TERMINAL_STRING, strlen(I_TERMINAL_STRING));
	input_init_buffer(&fix.input_rule_one, I_RULE_ONE, strlen(I_RULE_ONE));
	input_init_buffer(&fix.input_white, I_WHITE, strlen(I_WHITE));
	lexer_init(&fix.lexer_integer, &fix.input_integer, ebnf_lexer);
	lexer_init(&fix.lexer_identifier, &fix.input_identifier, ebnf_lexer);
	lexer_init(&fix.lexer_terminal_string, &fix.input_terminal_string, ebnf_lexer);
	lexer_init(&fix.lexer_rule_one, &fix.input_rule_one, ebnf_lexer);
	lexer_init(&fix.lexer_white, &fix.input_white, ebnf_lexer);

	//Utf8 tests
	input_init_buffer(&fix.input_utf8_two_byte, I_UTF8_TWO_BYTE, strlen(I_UTF8_TWO_BYTE));
	lexer_init(&fix.lexer_utf8_two_byte, &fix.input_utf8_two_byte, utf8_lexer);
	input_init_buffer(&fix.input_utf8_three_byte, I_UTF8_THREE_BYTE, strlen(I_UTF8_THREE_BYTE));
	lexer_init(&fix.lexer_utf8_three_byte, &fix.input_utf8_three_byte, utf8_lexer);
}

void t_teardown(){
	input_dispose(&fix.input_integer);
	input_dispose(&fix.input_identifier);
	input_dispose(&fix.input_terminal_string);
		input_dispose(&fix.input_rule_one);
		input_dispose(&fix.input_white);
	}

	void lexer_input_next__integer_token(){
		lexer_next(&fix.lexer_integer, &fix.token);
		t_assert(fix.token.symbol == E_INTEGER);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == strlen(I_INTEGER));
	}

	void lexer_input_next__identifier_token(){
		lexer_next(&fix.lexer_identifier, &fix.token);
		t_assert(fix.token.symbol == E_META_IDENTIFIER);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == strlen(I_IDENTIFIER));
	}

	void lexer_input_next__terminal_string_token(){
		lexer_next(&fix.lexer_terminal_string, &fix.token);
		t_assert(fix.token.symbol == E_TERMINAL_STRING);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == strlen(I_TERMINAL_STRING));
	}

	void lexer_input_next__skip_white_space(){
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_META_IDENTIFIER);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == 3);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_DEFINING_SYMBOL);
		t_assert(fix.token.index == 4);
		t_assert(fix.token.length == 1);
	}

	void lexer_input_next__whole_rule(){
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_META_IDENTIFIER);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == 3);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_DEFINING_SYMBOL);
		t_assert(fix.token.index == 4);
		t_assert(fix.token.length == 1);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_TERMINAL_STRING);
		t_assert(fix.token.index == 6);
		t_assert(fix.token.length == 3);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_CONCATENATE_SYMBOL);
		t_assert(fix.token.index == 9);
		t_assert(fix.token.length == 1);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_START_GROUP_SYMBOL);
		t_assert(fix.token.index == 10);
		t_assert(fix.token.length == 1);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_TERMINAL_STRING);
		t_assert(fix.token.index == 11);
		t_assert(fix.token.length == 3);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_DEFINITION_SEPARATOR_SYMBOL);
		t_assert(fix.token.index == 14);
		t_assert(fix.token.length == 1);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_TERMINAL_STRING);
		t_assert(fix.token.index == 15);
		t_assert(fix.token.length == 3);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == E_END_GROUP_SYMBOL);
		t_assert(fix.token.index == 18);
		t_assert(fix.token.length == 1);
		lexer_next(&fix.lexer_rule_one, &fix.token);
		t_assert(fix.token.symbol == L_EOF);
		t_assert(fix.token.index == 19);
		t_assert(fix.token.length == 0);
	}

	void lexer_input_next__white_token(){
		lexer_next(&fix.lexer_white, &fix.token);
		t_assert(fix.token.symbol == E_META_IDENTIFIER);
		t_assert(fix.token.index == 5);
		t_assert(fix.token.length == strlen("identifier"));
	}

	void lexer_input_next__utf8_two_byte(){
		lexer_next(&fix.lexer_utf8_two_byte, &fix.token);
		t_assert(fix.token.symbol == 0xF1);
		t_assert(fix.token.index == 0);
		t_assert(fix.token.length == 2);
	}

	void lexer_input_next__utf8_three_byte(){
		lexer_next(&fix.lexer_utf8_three_byte, &fix.token);
	t_assert(fix.token.symbol == 0xF71);
	t_assert(fix.token.index == 0);
	t_assert(fix.token.length == 3);
}

int main(int argc, char** argv){
	t_init();
	t_test(lexer_input_next__integer_token);
	t_test(lexer_input_next__identifier_token);
	t_test(lexer_input_next__terminal_string_token);
	t_test(lexer_input_next__skip_white_space);
	t_test(lexer_input_next__whole_rule);
	t_test(lexer_input_next__white_token);

	t_test(lexer_input_next__utf8_two_byte);
	t_test(lexer_input_next__utf8_three_byte);
	return t_done();
}
