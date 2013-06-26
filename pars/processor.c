#include "processor.h"

int reduce_handler(void *target, void *args) {
	Processor *proc = (Processor *)target;
	ReduceArgs *red = (ReduceArgs *)args;

	ast_add(proc->ast, red->symbol, red->index, red->length);
}

void processor_init(Processor *processor, Fsm *fsm)
{
	processor->fsm = fsm;
	processor->reduce_listener = fsm;
}

void processor_run(Processor *processor, LInput *input)
{
    Session *session = fsm_start_session(processor->fsm);
	session_on_reduce(session, processor->reduce_listener);
    while (!input->eof) {
		LToken token = lexer_input_next(input);
		session_match(session, token, lexer_input_get_index(input));
    }
}
