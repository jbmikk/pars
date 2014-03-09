#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "event.h"
#include "lexer.h"
#include "fsm.h"
#include "ast.h"

typedef struct _Processor {
    Fsm *fsm;
	Ast ast;
    EventListener fsm_listener;
} Processor;

void processor_init(Processor *processor, Fsm *fsm, EventListener listener);
void processor_run(Processor *processor, LInput *input);
int processor_fsm_ast_mapper(int type, void *target, void *args);

#endif //PROCESSOR_H
