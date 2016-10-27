#include <stddef.h>

#include "cli.h"
#include "symbols.h"
#include "fsm.h"
#include "test.h"


void t_setup(){
}

void t_teardown(){
}

void test_load_grammar(){
	SymbolTable table;
	Fsm fsm;

	symbol_table_init(&table);
	fsm_init(&fsm, &table);

	int error = cli_load_grammar("not-a-valid-file-name", &fsm);

	fsm_dispose(&fsm);
	symbol_table_dispose(&table);

	t_assert(error <= 0);
}

int main(int argc, char** argv){
	t_init();
	t_test(test_load_grammar);
	return t_done();
}
