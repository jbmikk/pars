#include <stddef.h>
#include <string.h>

#include "source.h"
#include "test.h"

void t_setup(){
}

void t_teardown(){
}

void source_init__existent_file(){
	Source source;
	source_init(&source);
	int error = source_open_file(&source, "grammars/test_ebnf_grammar.txt");
	t_assert(!error);
	source_dispose(&source);
}

void source_init__missing_file(){
	Source source;
	source_init(&source);
	int error = source_open_file(&source, "should_not_exist.txt");
	t_assert(error);
	source_dispose(&source);
}

int main(int argc, char** argv){
	t_init();
	t_test(source_init__existent_file);
	t_test(source_init__missing_file);
	return t_done();
}
