#include <stddef.h>
#include <string.h>

#include "input.h"
#include "test.h"

void t_setup(){
}

void t_teardown(){
}

void input_init__existent_file(){
	Input input;
	input_init(&input);
	int error = input_open_file(&input, "grammars/test_ebnf_grammar.txt");
	t_assert(!error);
	input_dispose(&input);
}

void input_init__missing_file(){
	Input input;
	input_init(&input);
	int error = input_open_file(&input, "should_not_exist.txt");
	t_assert(error);
	input_dispose(&input);
}

int main(int argc, char** argv){
	t_init();
	t_test(input_init__existent_file);
	t_test(input_init__missing_file);
	return t_done();
}
