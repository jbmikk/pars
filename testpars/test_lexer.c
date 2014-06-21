#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "lexer.h"

typedef struct {
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
	lexer_init(&fix->lexer_integer, input_init_buffer(I_INTEGER, strlen(I_INTEGER)));
	lexer_init(&fix->lexer_identifier, input_init_buffer(I_IDENTIFIER, strlen(I_IDENTIFIER)));
	lexer_init(&fix->lexer_terminal_string, input_init_buffer(I_TERMINAL_STRING, strlen(I_TERMINAL_STRING)));
	lexer_init(&fix->lexer_rule_one, input_init_buffer(I_RULE_ONE, strlen(I_RULE_ONE)));
}

void teardown(Fixture *fix, gconstpointer data){
	input_close(fix->lexer_integer.input);
	input_close(fix->lexer_identifier.input);
	input_close(fix->lexer_terminal_string.input);
	input_close(fix->lexer_rule_one.input);
}

void lexer_input_next__integer_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_integer);
	g_assert_cmpint(fix->lexer_integer.symbol, ==, L_INTEGER);
}

void lexer_input_next__identifier_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_identifier);
	g_assert_cmpint(fix->lexer_identifier.symbol, ==, L_IDENTIFIER);
}

void lexer_input_next__terminal_string_token(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_terminal_string);
	g_assert_cmpint(fix->lexer_terminal_string.symbol, ==, L_TERMINAL_STRING);
}

void lexer_input_next__skip_white_space(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_IDENTIFIER);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINING_SYMBOL);
}

void lexer_input_next__whole_rule(Fixture *fix, gconstpointer data){
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_IDENTIFIER);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINING_SYMBOL);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_CONCATENATE_SYMBOL);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_START_GROUP_SYMBOL);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_DEFINITION_SEPARATOR_SYMBOL);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_TERMINAL_STRING);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_END_GROUP_SYMBOL);
	lexer_next(&fix->lexer_rule_one);
	g_assert_cmpint(fix->lexer_rule_one.symbol, ==, L_EOF);
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
