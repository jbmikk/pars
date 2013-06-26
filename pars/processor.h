#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "event.h"
#include "lexer.h"
#include "fsm.h"
#include "ast.h"

typedef struct _Processor {
    Fsm *fsm;
	Ast ast;
    EventListener reduce_listener;
} Processor;

void processor_init(Processor *processor, Fsm *fsm);
void processor_run(Processor *processor, LInput *input);

#endif //PROCESSOR_H
