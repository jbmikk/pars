#include <stddef.h>

#include "cli.h"
#include "symbols.h"
#include "parser.h"
#include "test.h"


void t_setup(){
}

void t_teardown(){
}

void test_load_grammar(){
	Parser parser;

	parser_init(&parser);

	int error = cli_load_grammar("not-a-valid-file-name", &parser);

	parser_dispose(&parser);

	t_assert(error <= 0);
}

int main(int argc, char** argv){
	t_init();
	t_test(test_load_grammar);
	return t_done();
}
