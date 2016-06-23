#include <stddef.h>
#include <string.h>

#include "lexer.h"
#include "test.h"

typedef struct {
	Input input_integer;
	Input input_identifier;
	Input input_terminal_string;
	Input input_rule_one;
	Lexer lexer_integer;
	Lexer lexer_identifier;
	Lexer lexer_terminal_string;
	Lexer lexer_rule_one;
} Fixture;

Fixture fix;

#define I_INTEGER "1234"
#define I_IDENTIFIER "anIdentifier"
#define I_TERMINAL_STRING "\"terminalString\""
#define I_RULE_ONE "one = \"1\",(\"a\"|\"b\")"

void t_setup(){
	input_init_buffer(&fix.input_integer, I_INTEGER, strlen(I_INTEGER));
	input_init_buffer(&fix.input_identifier, I_IDENTIFIER, strlen(I_IDENTIFIER));
	input_init_buffer(&fix.input_terminal_string, I_TERMINAL_STRING, strlen(I_TERMINAL_STRING));
	input_init_buffer(&fix.input_rule_one, I_RULE_ONE, strlen(I_RULE_ONE));
	lexer_init(&fix.lexer_integer, &fix.input_integer, ebnf_lexer);
	lexer_init(&fix.lexer_identifier, &fix.input_identifier, ebnf_lexer);
	lexer_init(&fix.lexer_terminal_string, &fix.input_terminal_string, ebnf_lexer);
	lexer_init(&fix.lexer_rule_one, &fix.input_rule_one, ebnf_lexer);
}

void t_teardown(){
	input_dispose(&fix.input_integer);
	input_dispose(&fix.input_identifier);
	input_dispose(&fix.input_terminal_string);
	input_dispose(&fix.input_rule_one);
}

void lexer_input_next__integer_token(){
	lexer_next(&fix.lexer_integer);
	t_assert(fix.lexer_integer.symbol == L_INTEGER);
	t_assert(fix.lexer_integer.index == 0);
	t_assert(fix.lexer_integer.length == strlen(I_INTEGER));
}

void lexer_input_next__identifier_token(){
	lexer_next(&fix.lexer_identifier);
	t_assert(fix.lexer_identifier.symbol == L_IDENTIFIER);
	t_assert(fix.lexer_identifier.index == 0);
	t_assert(fix.lexer_identifier.length == strlen(I_IDENTIFIER));
}

void lexer_input_next__terminal_string_token(){
	lexer_next(&fix.lexer_terminal_string);
	t_assert(fix.lexer_terminal_string.symbol == L_TERMINAL_STRING);
	t_assert(fix.lexer_terminal_string.index == 0);
	t_assert(fix.lexer_terminal_string.length == strlen(I_TERMINAL_STRING));
}

void lexer_input_next__skip_white_space(){
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_IDENTIFIER);
	t_assert(fix.lexer_rule_one.index == 0);
	t_assert(fix.lexer_rule_one.length == 3);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_DEFINING_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 4);
	t_assert(fix.lexer_rule_one.length == 1);
}

void lexer_input_next__whole_rule(){
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_IDENTIFIER);
	t_assert(fix.lexer_rule_one.index == 0);
	t_assert(fix.lexer_rule_one.length == 3);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_DEFINING_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 4);
	t_assert(fix.lexer_rule_one.length == 1);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_TERMINAL_STRING);
	t_assert(fix.lexer_rule_one.index == 6);
	t_assert(fix.lexer_rule_one.length == 3);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_CONCATENATE_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 9);
	t_assert(fix.lexer_rule_one.length == 1);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_START_GROUP_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 10);
	t_assert(fix.lexer_rule_one.length == 1);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_TERMINAL_STRING);
	t_assert(fix.lexer_rule_one.index == 11);
	t_assert(fix.lexer_rule_one.length == 3);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_DEFINITION_SEPARATOR_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 14);
	t_assert(fix.lexer_rule_one.length == 1);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_TERMINAL_STRING);
	t_assert(fix.lexer_rule_one.index == 15);
	t_assert(fix.lexer_rule_one.length == 3);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_END_GROUP_SYMBOL);
	t_assert(fix.lexer_rule_one.index == 18);
	t_assert(fix.lexer_rule_one.length == 1);
	lexer_next(&fix.lexer_rule_one);
	t_assert(fix.lexer_rule_one.symbol == L_EOF);
	t_assert(fix.lexer_rule_one.index == 19);
	t_assert(fix.lexer_rule_one.length == 0);
}

int main(int argc, char** argv){
	t_init();
	t_test(lexer_input_next__integer_token);
	t_test(lexer_input_next__identifier_token);
	t_test(lexer_input_next__terminal_string_token);
	t_test(lexer_input_next__skip_white_space);
	t_test(lexer_input_next__whole_rule);
	return t_done();
}
