#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "ebnf_parser.h"
#include "fsmthread.h"
#include "ctest.h"


#define MATCH_DROP(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_DROP);

#define MATCH_SHIFT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_SHIFT);

#define MATCH_REDUCE(S, Y, R) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_REDUCE); \
	t_assert(tran.action->reduction == R);

#define MATCH_POP(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_POP);

#define MATCH_POP_SHIFT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_POP_SHIFT);

#define MATCH_ACCEPT(S, Y) \
	tran = fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)}); \
	fsm_thread_apply(&(S), tran); \
	t_assert(tran.action->type == ACTION_ACCEPT);


#define nzs(S) (S), (strlen(S))

typedef struct {
	SymbolTable table;
	Fsm fsm;
	Fsm lexer_fsm;

	int INTEGER;
	int IDENTIFIER;
	int STRING;
	int END_BLOCK_SYMBOL;
	int START_BLOCK_SYMBOL;
	int BODY;
	int SELECTOR;
	int ATL_RULE;
	int SYNTAX;
} Fixture;

Fixture fix;

int *current;
unsigned int token_index;
unsigned int diff;
unsigned int count;

void h(int token)
{
	if (current[token_index++] != token)
		diff++;
	count++;
}

void t_setup(){
	token_index = 0;
	diff = 0;
	count = 0;
	symbol_table_init(&fix.table);

	fsm_init(&fix.lexer_fsm, &fix.table);
	atl_build_lexer_fsm(&fix.lexer_fsm);

	fsm_init(&fix.fsm, &fix.table);
	atl_build_fsm(&fix.fsm);

	fix.SYNTAX = fsm_get_symbol_id(&fix.fsm, nzs("syntax"));
	fix.ATL_RULE = fsm_get_symbol_id(&fix.fsm, nzs("atl_rule"));
	fix.SELECTOR = fsm_get_symbol_id(&fix.fsm, nzs("selector"));
	fix.BODY = fsm_get_symbol_id(&fix.fsm, nzs("body"));
	fix.IDENTIFIER = fsm_get_symbol_id(fsm, nzs("identifier"));
	fix.INTEGER = fsm_get_symbol_id(fsm, nzs("integer"));
	fix.STRING = fsm_get_symbol_id(fsm, nzs("string"));
	fix.START_BLOCK_SYMBOL = fsm_get_symbol_id(fsm, nzs("start_block_symbol"));
	fix.END_BLOCK_SYMBOL = fsm_get_symbol_id(fsm, nzs("end_block_symbol"));
}

void t_teardown(){
	fsm_dispose(&fix.fsm);
	fsm_dispose(&fix.lexer_fsm);
	symbol_table_dispose(&fix.table);
}

void ebnf_start_parsing__syntax(){

	Transition tran;
	FsmThread thread;
	fsm_thread_init(&thread, &fix.fsm, (Listener) { .function = NULL });
	fsm_thread_start(&thread);

	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_DROP(thread, fix.DEFINING_SYMBOL);
	MATCH_SHIFT(thread, fix.META_IDENTIFIER);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_PRIMARY);
	MATCH_POP_SHIFT(thread, fix.TERMINATOR_SYMBOL);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_FACTOR);
	MATCH_POP_SHIFT(thread, fix.TERMINATOR_SYMBOL);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SYNTACTIC_TERM);
	MATCH_POP_SHIFT(thread, fix.TERMINATOR_SYMBOL);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.SINGLE_DEFINITION);
	MATCH_POP_SHIFT(thread, fix.TERMINATOR_SYMBOL);
	MATCH_REDUCE(thread, fix.TERMINATOR_SYMBOL, fix.DEFINITIONS_LIST);
	MATCH_POP(thread, fix.TERMINATOR_SYMBOL);
	MATCH_DROP(thread, fix.TERMINATOR_SYMBOL);

	MATCH_REDUCE(thread, L_EOF, fix.ATL_RULE);
	MATCH_POP(thread, fix.ATL_RULE);
	MATCH_REDUCE(thread, L_EOF, fix.SYNTAX);
	MATCH_POP(thread, fix.SYNTAX);

	MATCH_DROP(thread, L_EOF);

	fsm_thread_dispose(&thread);
}

int main(int argc, char** argv){
	t_init(&argc, &argv, NULL);
	t_test(ebnf_start_parsing__syntax);
	return t_done();
}
