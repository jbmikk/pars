#ifndef CLI_H
#define CLI_H

#include "fsm.h"
#include "ebnf_parser.h"

#include <stddef.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

typedef struct _Params {
	char *param;
	//TODO: Add flags for -v etc
	//int flags;
} Params;

int cli_load_grammar(Params *params, Parser *parser);

#endif //CLI_H
