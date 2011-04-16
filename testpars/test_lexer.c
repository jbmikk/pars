#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "lexer.h"

typedef struct {
    LInput *input_integer;
    LInput *input_identifier;
    LInput *input_terminal_string;
    LInput *input_rule_one;
} Fixture;

#define I_INTEGER "1234"
#define I_IDENTIFIER "anIdentifier"
#define I_TERMINAL_STRING "\"terminalString\""
#define I_RULE_ONE "one = \"1\",(\"a\"|\"b\")"

void setup(Fixture *fix, gconstpointer data){
    fix->input_integer = lexer_input_init_buffer(I_INTEGER, strlen(I_INTEGER));
    fix->input_identifier = lexer_input_init_buffer(I_IDENTIFIER, strlen(I_IDENTIFIER));
    fix->input_terminal_string = lexer_input_init_buffer(I_TERMINAL_STRING, strlen(I_TERMINAL_STRING));
    fix->input_rule_one = lexer_input_init_buffer(I_RULE_ONE, strlen(I_RULE_ONE));
}

void teardown(Fixture *fix, gconstpointer data){
    lexer_input_close(fix->input_integer);
    lexer_input_close(fix->input_identifier);
    lexer_input_close(fix->input_rule_one);
}

void lexer_input_init__existent_file(Fixture *fix, gconstpointer data){
    LInput *input = lexer_input_init("test_ebnf_grammar.txt");
    g_assert(input != NULL);
    lexer_input_close(input);
}

void lexer_input_init__missing_file(Fixture *fix, gconstpointer data){
    LInput *input = lexer_input_init("should_not_exist.txt");
    g_assert(input == NULL);
}

void lexer_input_next__integer_token(Fixture *fix, gconstpointer data){
    g_assert_cmpint(lexer_input_next(fix->input_integer), ==, L_INTEGER);
}

void lexer_input_next__identifier_token(Fixture *fix, gconstpointer data){
    g_assert_cmpint(lexer_input_next(fix->input_identifier), ==, L_IDENTIFIER);
}

void lexer_input_next__terminal_string_token(Fixture *fix, gconstpointer data){
    g_assert_cmpint(lexer_input_next(fix->input_terminal_string), ==, L_TERMINAL_STRING);
}

void lexer_input_next__skip_white_space(Fixture *fix, gconstpointer data){
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_IDENTIFIER);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_DEFINING_SYMBOL);
}

void lexer_input_next__whole_rule(Fixture *fix, gconstpointer data){
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_IDENTIFIER);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_DEFINING_SYMBOL);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_TERMINAL_STRING);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_CONCATENATE_SYMBOL);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_START_GROUP_SYMBOL);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_TERMINAL_STRING);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_DEFINITION_SEPARATOR_SYMBOL);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_TERMINAL_STRING);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_END_GROUP_SYMBOL);
    g_assert_cmpint(lexer_input_next(fix->input_rule_one), ==, L_EOF);
}

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/Lexer/input_init", Fixture, NULL, setup, lexer_input_init__existent_file, teardown);
    g_test_add("/Lexer/input_init", Fixture, NULL, setup, lexer_input_init__missing_file, teardown);
    g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__integer_token, teardown);
    g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__identifier_token, teardown);
    g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__terminal_string_token, teardown);
    g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__skip_white_space, teardown);
    g_test_add("/Lexer/input_next", Fixture, NULL, setup, lexer_input_next__whole_rule, teardown);
    return g_test_run();
}
