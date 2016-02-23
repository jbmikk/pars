#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "lexer.h"

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

#define I_INTEGER "1234"
#define I_IDENTIFIER "anIdentifier"
#define I_TERMINAL_STRING "\"terminalString\""
#define I_RULE_ONE "one = \"1\",(\"a\"|\"b\")"

void setup(Fixture *fix, gconstpointer data){
	input_init_buffer(&fix->input_integer, I_INTEGER, strlen(I_INTEGER));
	input_init_buffer(&fix->input_identifier, I_IDENTIFIER, strlen(I_IDENTIFIER));
	input_init_buffer(&fix->input_terminal_string, I_TERMINAL_STRING, strlen(I_TERMINAL_STRING));
	input_init_buffer(&fix->input_rule_one, I_RULE_ONE, strlen(I_RULE_ONE));
	lexer_init(&fix->lexer_integer, &fix->input_integer, ebnf_lexer);
	lexer_init(&fix->lexer_identifier, &fix->input_identifier, ebnf_lexer);
	lexer_init(&fix->lexer_terminal_string, &fix->input_terminal_string, ebnf_lexer);
	lexer_init(&fix->lexer_rule_one, &fix->input_rule_one, ebnf_lexer);
}

void teardown(Fixture *fix, gconstpointer data){
	input_dispose(&fix->input_integer);
	input_dispose(&fix->input_identifier);
	input_dispose(&fix->input_terminal_string);
	input_dispose(&fix->input_rule_one);
}

void lexer_input_next__integer_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_integer);
	g_assert_cmpint(fix->lexer_integer.symbol, ==, L_INTEGER);
	g_assert_cmpint(fix->lexer_integer.index, ==, 0);
	g_assert_cmpint(fix->lexer_integer.length, ==, strlen(I_INTEGER));
}

void lexer_input_next__identifier_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_identifier);
	g_assert_cmpint(fix->lexer_identifier.symbol, ==, L_IDENTIFIER);
	g_assert_cmpint(fix->lexer_identifier.index, ==, 0);
	g_assert_cmpint(fix->lexer_identifier.length, ==, strlen(I_IDENTIFIER));
}

void lexer_input_next__terminal_string_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_terminal_string);
	g_assert_cmpint(fix->lexer_terminal_string.symbol, ==, L_TERMINAL_STRING);
	g_assert_cmpint(fix->lexer_terminal_string.index, ==, 0);
	g_assert_cmpint(fix->lexer_terminal_string.length, ==, strlen(I_TERMINAL_STRING));
}

void lexer_input_next__skip_white_space(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_IDENTIFIER);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 0);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 3);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINING_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 4);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
}

void lexer_input_next__whole_rule(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_IDENTIFIER);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 0);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 3);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINING_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 4);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 6);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 3);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_CONCATENATE_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 9);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_START_GROUP_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 10);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 11);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 3);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINITION_SEPARATOR_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 14);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 15);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 3);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_END_GROUP_SYMBOL);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 18);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 1);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_EOF);
	g_assert_cmpint(fix->lexer_rule_one.index, ==, 19);
	g_assert_cmpint(fix->lexer_rule_one.length, ==, 0);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__integer_token, teardown);
	g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__identifier_token, teardown);
	g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__terminal_string_token, teardown);
	g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__skip_white_space, teardown);
	g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__whole_rule, teardown);
	return g_test_run();
}
