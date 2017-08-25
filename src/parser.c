#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

void parser_init(Parser *parser)
{
	symbol_table_init(&parser->table);
	fsm_init(&parser->lexer_fsm, &parser->table);
	fsm_init(&parser->fsm, &parser->table);
}

void parser_dispose(Parser *parser)
{
	fsm_dispose(&parser->fsm);
	fsm_dispose(&parser->lexer_fsm);
	symbol_table_dispose(&parser->table);
}


