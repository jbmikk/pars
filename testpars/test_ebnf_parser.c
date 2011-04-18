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

void setup(Fixture *fix, gconstpointer data){
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
    g_assert_cmpint(ebnf_start_parsing(fix->input_identifier), ==, 0);
}

void ebnf_start_parsing__terminal(Fixture *fix, gconstpointer data){
    g_assert_cmpint(ebnf_start_parsing(fix->input_terminal), ==, 0);
}

void ebnf_start_parsing__concatenate(Fixture *fix, gconstpointer data){
    g_assert_cmpint(ebnf_start_parsing(fix->input_concatenate), ==, 0);
}

void ebnf_start_parsing__separator(Fixture *fix, gconstpointer data){
    g_assert_cmpint(ebnf_start_parsing(fix->input_separator), ==, 0);
}

void ebnf_start_parsing__two_rules(Fixture *fix, gconstpointer data){
    g_assert_cmpint(ebnf_start_parsing(fix->input_two_rules), ==, 0);
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
