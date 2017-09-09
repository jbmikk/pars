#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "fsmbuilder.h"
#include "parsercontext.h"
#include "astbuilder.h"
#include "controlloop.h"
#include "test.h"

#define MATCH(S, Y) fsm_thread_match(&(S), &(struct _Token){ 0, 0, (Y)});
#define TEST(S, Y) fsm_thread_test(&(S), &(struct _Token){ 0, 0, (Y)});

#define nzs(S) (S), (strlen(S))

typedef struct {
	Parser parser;
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

static void _test_identity_init_lexer_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	fsm_builder_init(&builder, fsm);

	fsm_builder_set_mode(&builder, nzs(".default"));

	fsm_builder_lexer_done(&builder, L_EOF);
	fsm_builder_lexer_default_input(&builder);

	fsm_builder_dispose(&builder);
}

static void _test_build_tree_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	Symbol *t_down = symbol_table_get(&fix.parser.table, "__tdown", 7);
	//Symbol *t_up = symbol_table_get(&fix.parser.table, "__tup", 5);

	fsm_builder_init(&builder, fsm);

	fsm_builder_define(&builder, nzs("rule"));
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

static void _test_pipe_token(void *thread, const Token *token)
{
	fsm_thread_match((FsmThread *)thread, token);
}

static int _test_setup_lexer(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->lexer_thread.handler.target = &context->thread;
	context->lexer_thread.handler.drop = NULL;
	context->lexer_thread.handler.shift = NULL;
	context->lexer_thread.handler.reduce = NULL;
	context->lexer_thread.handler.accept = _test_pipe_token;
	return 0;
}

void _on_shift(void *ast_p, const Token *token)
{
}

void _on_reduce(void *ast_p, const Token *token)
{
}

static int _test_setup_fsm(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->thread.handler.target = context->ast;
	context->thread.handler.shift = _on_shift;
	context->thread.handler.reduce = _on_reduce;
	context->thread.handler.accept = NULL;

	return 0;
}

static int _test_parse_start(void *object, void *params)
{
	//ParserContext *context = (ParserContext *)object;
	return 0;
}

static int _test_parse_end(void *object, void *params)
{
	//ParserContext *context = (ParserContext *)object;
	return 0;
}

static int _test_parse_error(void *object, void *params)
{
	//ParserContext *context = (ParserContext *)object;
	return 0;
}

void t_setup(){
	parser_init(&fix.parser);
	listener_init(&fix.parser.parse_setup_lexer, _test_setup_lexer, NULL);
	listener_init(&fix.parser.parse_setup_fsm, _test_setup_fsm, NULL);
	listener_init(&fix.parser.parse_start, _test_parse_start, NULL);
	listener_init(&fix.parser.parse_loop, control_loop_ast, NULL);
	listener_init(&fix.parser.parse_end, _test_parse_end, NULL);
	listener_init(&fix.parser.parse_error, _test_parse_error, NULL);

	symbol_table_add(&fix.parser.table, "__tdown", 7);
	symbol_table_add(&fix.parser.table, "__tup", 5);

	_test_identity_init_lexer_fsm(&fix.parser.lexer_fsm);
	_test_build_tree_fsm(&fix.parser.fsm);
}

void t_teardown(){
	parser_dispose(&fix.parser);
}

void parser_basic_parse(){

	ParserContext context;
	Ast ast;
	AstBuilder builder;

	ast_init(&ast, NULL, &fix.parser.table);

	ast_builder_init(&builder, &ast);
	ast_builder_open(&builder, &(Token){1, 1, 0});
	ast_builder_open(&builder, &(Token){2, 1, 0});
	ast_builder_close(&builder, &(Token){3, 1, 'b'});
	ast_builder_close(&builder, &(Token){6, 5, 'a'});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	parser_context_init(&context, &fix.parser);

	parser_context_set_input_ast(&context, &ast);

	int error = parser_context_execute(&context);

	t_assert(!error);

	parser_context_dispose(&context);
	ast_dispose(&ast);
}

int main(int argc, char** argv){
	t_init(&argc, &argv, NULL);
	t_test(parser_basic_parse);
	return t_done();
}