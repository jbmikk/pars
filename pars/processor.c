#include "processor.h"

void processor_init(Processor *processor, Fsm *fsm)
{
	processor->fsm = fsm;
}

void processor_run(Processor *processor, LInput *input)
{
    Session *session = fsm_start_session(processor->fsm);
    while (!input->eof) {
		LToken token = lexer_input_next(input);
		session_match(session, token, lexer_input_get_index(input));
    }
}
