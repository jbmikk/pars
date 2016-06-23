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
	input_init(&input, "grammars/test_ebnf_grammar.txt");
	t_assert(input.is_open);
	input_dispose(&input);
}

void input_init__missing_file(){
	Input input;
	input_init(&input, "should_not_exist.txt");
	t_assert(!input.is_open);
	input_dispose(&input);
}

int main(int argc, char** argv){
	t_init();
	t_test(input_init__existent_file);
	t_test(input_init__missing_file);
	return t_done();
}
