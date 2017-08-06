#ifndef CLI_H
#define CLI_H

#include "fsm.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

int cli_load_grammar(char *pathname, Parser *parser);

#endif //CLI_H
