#include "parser.h"

#include "cmemory.h"
#include "dbg.h"

void parser_init(Parser *parser)
{
	symbol_table_init(&parser->table);
	fsm_init(&parser->lexer_fsm, &parser->table);
	fsm_init(&parser->fsm, &parser->table);
	fsm_thread_init(&parser->proto_fsm_thread, &parser->fsm);
	fsm_thread_init(&parser->proto_lexer_fsm_thread, &parser->lexer_fsm);
}

void parser_dispose(Parser *parser)
{
	fsm_thread_dispose(&parser->proto_fsm_thread);
	fsm_thread_dispose(&parser->proto_lexer_fsm_thread);
	fsm_dispose(&parser->fsm);
	fsm_dispose(&parser->lexer_fsm);
	symbol_table_dispose(&parser->table);
}

void parser_setup_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->proto_fsm_thread.handler.target = NULL;
	parser->proto_fsm_thread.handler.shift = shift;
	parser->proto_fsm_thread.handler.reduce = reduce;
	parser->proto_fsm_thread.handler.accept = accept;
}

void parser_setup_lexer_fsm(
	Parser *parser,
	void (*shift)(void *target, const Token *token),
	void (*reduce)(void *target, const Token *token),
	void (*accept)(void *target, const Token *token)
) {
	parser->proto_lexer_fsm_thread.handler.target = NULL;
	parser->proto_lexer_fsm_thread.handler.shift = shift;
	parser->proto_lexer_fsm_thread.handler.reduce = reduce;
	parser->proto_lexer_fsm_thread.handler.accept = accept;
}

