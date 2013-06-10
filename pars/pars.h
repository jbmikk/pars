#ifndef PARS_H
#define PARS_H

#include "fsm.h"
#include "ebnf_parser.h"
#include "processor.h"

#include <stddef.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

Processor *pars_load_grammar(char *pathname);

#endif //PARS_H
