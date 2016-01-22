#include <stddef.h>
#include <glib.h>

#include "pars.h"

typedef struct {
	int placeholder;
} Fixture;

void pars_setup(Fixture *fix, gconstpointer data){
}

void pars_teardown(Fixture *fix, gconstpointer data){
}

void test_load_grammar(Fixture *fix, gconstpointer data){
	pars_load_grammar(NULL);
	g_assert_cmpstr("bla", ==, "bla");
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Pars/pars_load_grammar", Fixture, NULL, pars_setup, test_load_grammar, pars_teardown);
	return g_test_run();
}
