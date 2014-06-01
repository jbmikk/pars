#include <stddef.h>
#include <string.h>
#include <glib.h>

#include "input.h"

typedef struct {
} Fixture;

void setup(Fixture *fix, gconstpointer data){
}

void teardown(Fixture *fix, gconstpointer data){
}

void input_init__existent_file(Fixture *fix, gconstpointer data){
    Input *input = input_init("grammars/test_ebnf_grammar.txt");
    g_assert(input != NULL);
    input_close(input);
}

void input_init__missing_file(Fixture *fix, gconstpointer data){
    Input *input = input_init("should_not_exist.txt");
    g_assert(input == NULL);
}

int main(int argc, char** argv){
    g_test_init(&argc, &argv, NULL);
    g_test_add("/Input/input_init", Fixture, NULL, setup, input_init__existent_file, teardown);
    g_test_add("/Input/input_init", Fixture, NULL, setup, input_init__missing_file, teardown);
    return g_test_run();
}
