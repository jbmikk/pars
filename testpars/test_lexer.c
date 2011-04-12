#include <stddef.h>
#include <glib.h>

#include "lexer.h"

typedef struct {
    int placeholder;
} Fixture;

void setup(Fixture *fix, gconstpointer data){
}

void teardown(Fixture *fix, gconstpointer data){
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

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/Lexer/input_init", Fixture, NULL, setup, lexer_input_init__existent_file, teardown);
    g_test_add("/Lexer/input_init", Fixture, NULL, setup, lexer_input_init__missing_file, teardown);
    return g_test_run();
}
