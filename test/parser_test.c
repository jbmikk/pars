#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "fsmbuilder.h"
#include "parsercontext.h"
#include "astbuilder.h"
#include "controlloop.h"
#include "test.h"

#define MATCH(S, Y) fsm_thread_match(&(S), &(struct Token){ 0, 0, (Y)});
#define TEST(S, Y) fsm_thread_test(&(S), &(struct Token){ 0, 0, (Y)});

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

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	// TODO: Is this still necessary? lexer_done already handles it.
	fsm_builder_set_mode(&builder, nzs(".default"));

	fsm_builder_define(&builder, nzs("any_char"));
	fsm_builder_terminal_range(&builder, (Range){ 1, 256 });
	fsm_builder_identity(&builder);
	fsm_builder_end(&builder);

	fsm_builder_lexer_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

static void _build_simple_ab_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	Symbol *t_down = symbol_table_get(&fix.parser.table, "__tdown", 7);
	//Symbol *t_up = symbol_table_get(&fix.parser.table, "__tup", 5);

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	fsm_builder_define(&builder, nzs("rule"));
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

static void _build_simple_abc_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	Symbol *t_down = symbol_table_get(&fix.parser.table, "__tdown", 7);
	//Symbol *t_up = symbol_table_get(&fix.parser.table, "__tup", 5);

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	fsm_builder_define(&builder, nzs("rule"));
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_terminal(&builder, 'c');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

static void _build_child_rule_fsm(Fsm *fsm)
{
	FsmBuilder builder;

	Symbol *t_down = symbol_table_get(&fix.parser.table, "__tdown", 7);

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	fsm_builder_define(&builder, nzs("child"));
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_terminal(&builder, 'c');
	fsm_builder_end(&builder);

	fsm_builder_define(&builder, nzs("main"));
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_nonterminal(&builder, nzs("child"));
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

static void _build_siblings_with_children(Fsm *fsm)
{
	FsmBuilder builder;

	Symbol *t_down = symbol_table_get(&fix.parser.table, "__tdown", 7);
	Symbol *t_up = symbol_table_get(&fix.parser.table, "__tup", 5);

	fsm_builder_init(&builder, fsm, REF_STRATEGY_MERGE);

	fsm_builder_define(&builder, nzs("rule"));
	fsm_builder_terminal(&builder, 0);
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'a');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'b');
	fsm_builder_terminal(&builder, 'c');
	fsm_builder_terminal(&builder, t_up->id);
	fsm_builder_terminal(&builder, 'd');
	fsm_builder_terminal(&builder, t_down->id);
	fsm_builder_terminal(&builder, 'e');
	fsm_builder_terminal(&builder, 'f');
	fsm_builder_end(&builder);

	fsm_builder_done(&builder, L_EOF);

	fsm_builder_dispose(&builder);
}

int test_lexer_pipe(void *_context, void *_tran)
{
	ParserContext *context = (ParserContext *)_context;
	Transition *tran = (Transition *)_tran;

	if(tran->action->type != ACTION_ACCEPT) {
		return 0;
	}
	fsm_thread_match(&context->thread, &tran->reduction);
	return 0;
}

int test_parser_pipe(void *_context, void *_cont)
{
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

static void _test_build_parser_context(ParserContext *context)
{
	listener_init(&context->parse_setup_lexer, NULL, context);
	listener_init(&context->parse_setup_fsm, NULL, context);
	listener_init(&context->parse_start, _test_parse_start, context);
	listener_init(&context->parse_loop, control_loop_ast, context);
	listener_init(&context->parse_end, _test_parse_end, context);
	listener_init(&context->parse_error, _test_parse_error, context);

	listener_init(&context->lexer_pipe, test_lexer_pipe, context);
	listener_init(&context->parser_pipe, test_parser_pipe, context);
}


void t_setup(){
	parser_init(&fix.parser);

	//TODO: Move to _test_build_symbol_table?
	symbol_table_add(&fix.parser.table, "__tdown", 7);
	symbol_table_add(&fix.parser.table, "__tup", 5);

	_test_identity_init_lexer_fsm(&fix.parser.lexer_fsm);

	fix.parser.build_context = _test_build_parser_context;
}

void t_teardown(){
	parser_dispose(&fix.parser);
}

void _parser_basic_parse(){

	ParserContext context;
	Ast ast;
	AstBuilder builder;

	_build_simple_ab_fsm(&fix.parser.fsm);

	ast_init(&ast, NULL, &fix.parser.table);

	ast_builder_init(&builder, &ast);
	ast_builder_append_follow(&builder, &(Token){1, 1, 'a'});
	ast_builder_append(&builder, &(Token){2, 1, 'b'});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	parser_context_init(&context, &fix.parser);

	parser_context_set_source_ast(&context, &ast);

	int error = parser_context_execute(&context);

	t_assert(!error);

	parser_context_dispose(&context);
	ast_dispose(&ast);
}

static void _parse_parent_and_siblings_test(){

	ParserContext context;
	Ast ast;
	AstBuilder builder;

	_build_simple_abc_fsm(&fix.parser.fsm);

	ast_init(&ast, NULL, &fix.parser.table);

	ast_builder_init(&builder, &ast);
	ast_builder_append_follow(&builder, &(Token){1, 1, 'a'});
	ast_builder_append(&builder, &(Token){2, 1, 'b'});
	ast_builder_append(&builder, &(Token){3, 1, 'c'});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	parser_context_init(&context, &fix.parser);

	parser_context_set_source_ast(&context, &ast);

	int error = parser_context_execute(&context);

	t_assert(!error);

	parser_context_dispose(&context);
	ast_dispose(&ast);
}

static void _parse_sibling_with_children_test(){

	ParserContext context;
	Ast ast;
	AstBuilder builder;

	_build_siblings_with_children(&fix.parser.fsm);

	ast_init(&ast, NULL, &fix.parser.table);

	ast_builder_init(&builder, &ast);
	ast_builder_append_follow(&builder, &(Token){1, 1, 'a'});
	ast_builder_append(&builder, &(Token){2, 1, 'b'});
	ast_builder_append(&builder, &(Token){3, 1, 'c'});
	ast_builder_parent(&builder);
	ast_builder_append_follow(&builder, &(Token){4, 1, 'd'});
	ast_builder_append(&builder, &(Token){5, 1, 'e'});
	ast_builder_append(&builder, &(Token){6, 1, 'f'});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	parser_context_init(&context, &fix.parser);

	parser_context_set_source_ast(&context, &ast);

	int error = parser_context_execute(&context);

	t_assert(!error);

	parser_context_dispose(&context);
	ast_dispose(&ast);
}

static void _parse_parent_child_nonterminal(){

	ParserContext context;
	Ast ast;
	AstBuilder builder;

	_build_child_rule_fsm(&fix.parser.fsm);

	ast_init(&ast, NULL, &fix.parser.table);

	ast_builder_init(&builder, &ast);
	ast_builder_append_follow(&builder, &(Token){1, 1, 'a'});
	ast_builder_append(&builder, &(Token){2, 1, 'b'});
	ast_builder_append(&builder, &(Token){3, 1, 'c'});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	parser_context_init(&context, &fix.parser);

	parser_context_set_source_ast(&context, &ast);

	int error = parser_context_execute(&context);

	t_assert(!error);

	parser_context_dispose(&context);
	ast_dispose(&ast);
}

int main(int argc, char** argv){
	t_init(&argc, &argv, NULL);
	t_test(_parser_basic_parse);
	t_test(_parse_parent_and_siblings_test);
	t_test(_parse_sibling_with_children_test);
	t_test(_parse_parent_child_nonterminal);

	return t_done();
}
