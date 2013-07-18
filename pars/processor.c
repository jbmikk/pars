#include "processor.h"

void processor_init(Processor *processor, Fsm *fsm, EventListener listener)
{
	processor->fsm = fsm;
	processor->reduce_listener = listener;
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
