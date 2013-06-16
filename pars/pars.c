#include "pars.h"
#include "cmemory.h"
#include "lexer.h"
#include "fsm.h"
#include "ebnf_parser.h"
#include "processor.h"

#include <stddef.h>
#include <stdio.h>

void _pars_parse_grammar(Processor *processor, LInput *input)
{
    Fsm *fsm = c_new(Fsm, 1);
	init_ebnf_fsm(fsm);
	processor_init(processor, fsm);
	init_ebnf_interpreter(processor);
	processor_run(processor, input);
}

Processor *pars_load_grammar(char *pathname)
{
    LInput *input;
    Processor *processor = NULL;

    input = lexer_input_init(pathname);

	if(input) {
		if(input->is_open) {
			processor = c_new(Processor,1);
			_pars_parse_grammar(processor, input);
		} else {
			printf("Could not open grammar file\n");
		}
	} else {
		printf("Grammar file not found\n");
	}
    return processor;
}

#ifndef LIBRARY
int main(int argc, char** argv){
	if(argc > 1) {
		printf("Loading grammar\n");
		pars_load_grammar(argv[1]);
	}
    return 0;
}
#endif
