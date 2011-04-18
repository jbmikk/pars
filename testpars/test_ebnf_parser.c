#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "ebnf_parser.h"

typedef struct {
    LInput *input_identifier;
    LInput *input_terminal;
    LInput *input_concatenate;
    LInput *input_separator;
    LInput *input_two_rules;
} Fixture;

#define I_IDENTIFIER "rule = a;"
#define I_TERMINAL "rule = \"ab\";"
#define I_CONCATENATE "rule = a, b;"
#define I_SEPARATOR "rule = a | \"b\";"
#define I_TWO_RULES "rule1 = a;rule2 = b;"

#define EXPECTED(...) EToken exp[] = {__VA_ARGS__}; current = exp;

EToken *current;
unsigned int token_index;
unsigned int diff;
unsigned int count;

void h(int token)
{
    if (current[token_index++] != token)
        diff++;
    count++;
}

void setup(Fixture *fix, gconstpointer data){
    token_index = 0;
    diff = 0;
    count = 0;
    fix->input_identifier = lexer_input_init_buffer(I_IDENTIFIER, strlen(I_IDENTIFIER));
    fix->input_terminal = lexer_input_init_buffer(I_TERMINAL, strlen(I_TERMINAL));
    fix->input_concatenate = lexer_input_init_buffer(I_CONCATENATE, strlen(I_CONCATENATE));
    fix->input_separator = lexer_input_init_buffer(I_SEPARATOR, strlen(I_SEPARATOR));
    fix->input_two_rules = lexer_input_init_buffer(I_TWO_RULES, strlen(I_TWO_RULES));
}

void teardown(Fixture *fix, gconstpointer data){
    lexer_input_close(fix->input_identifier);
    lexer_input_close(fix->input_terminal);
    lexer_input_close(fix->input_concatenate);
    lexer_input_close(fix->input_separator);
    lexer_input_close(fix->input_two_rules);
}

void ebnf_start_parsing__identifier(Fixture *fix, gconstpointer data){
    EXPECTED(
        E_IDENTIFIER,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_SYNTAX
    )
    int ret = ebnf_start_parsing(fix->input_identifier, h);
    g_assert_cmpint(ret, ==, 0);
    g_assert_cmpint(count, ==, 5);
    g_assert_cmpint(diff, ==, 0);
}

void ebnf_start_parsing__terminal(Fixture *fix, gconstpointer data){
    EXPECTED(
        E_TERMINAL_STRING,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_SYNTAX
    )
    int ret = ebnf_start_parsing(fix->input_terminal, h);
    g_assert_cmpint(ret, ==, 0);
    g_assert_cmpint(count, ==, 5);
    g_assert_cmpint(diff, ==, 0);
}

void ebnf_start_parsing__concatenate(Fixture *fix, gconstpointer data){
    EXPECTED(
        E_IDENTIFIER,
        E_IDENTIFIER,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_SYNTAX
    )
    int ret = ebnf_start_parsing(fix->input_concatenate, h);
    g_assert_cmpint(ret, ==, 0);
    g_assert_cmpint(count, ==, 6);
    g_assert_cmpint(diff, ==, 0);
}

void ebnf_start_parsing__separator(Fixture *fix, gconstpointer data){
    EXPECTED(
        E_IDENTIFIER,
        E_SINGLE_DEFINITION,
        E_TERMINAL_STRING,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_SYNTAX
    )
    int ret = ebnf_start_parsing(fix->input_separator, h);
    g_assert_cmpint(ret, ==, 0);
    g_assert_cmpint(count, ==, 7);
    g_assert_cmpint(diff, ==, 0);
}

void ebnf_start_parsing__two_rules(Fixture *fix, gconstpointer data){
    EXPECTED(
        E_IDENTIFIER,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_IDENTIFIER,
        E_SINGLE_DEFINITION,
        E_DEFINITION_LIST,
        E_SYNTAX_RULE,
        E_SYNTAX
    )
    int ret = ebnf_start_parsing(fix->input_two_rules, h);
    g_assert_cmpint(ret, ==, 0);
    g_assert_cmpint(count, ==, 9);
    g_assert_cmpint(diff, ==, 0);
}

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__identifier, teardown);
    g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__terminal, teardown);
    g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__concatenate, teardown);
    g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__separator, teardown);
    g_test_add("/EBNF/start_parsing", Fixture, NULL, setup, ebnf_start_parsing__two_rules, teardown);
    return g_test_run();
}
