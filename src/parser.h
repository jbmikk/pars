#ifndef PARSER_H
#define PARSER_H

#include "fsm.h"
#include "listener.h"

typedef struct Parser {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;
	Listener parse_setup_fsm;
	Listener parse_setup_lexer;
	Listener parse_start;
	Listener parse_loop;
	Listener parse_end;
	Listener parse_error;
} Parser;

void parser_init(Parser *parser);
void parser_dispose(Parser *parser);

#endif //PARSER_H
