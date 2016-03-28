#include <stddef.h>
#include <glib.h>

#include "pars.h"
#include "symbols.h"
#include "fsm.h"

typedef struct {
	int placeholder;
} Fixture;

void pars_setup(Fixture *fix, gconstpointer data){
}

void pars_teardown(Fixture *fix, gconstpointer data){
}

void test_load_grammar(Fixture *fix, gconstpointer data){
	SymbolTable table;
	Fsm fsm;

	symbol_table_init(&table);
	fsm_init(&fsm, &table);

	int error = pars_load_grammar("not-a-valid-file-name", &fsm);

	fsm_dispose(&fsm);
	symbol_table_dispose(&table);

	g_assert_cmpint(error, <=, 0);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Pars/pars_load_grammar", Fixture, NULL, pars_setup, test_load_grammar, pars_teardown);
	return g_test_run();
}
