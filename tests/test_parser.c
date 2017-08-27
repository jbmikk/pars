#include <stddef.h>
#include <string.h>

#include "fsm.h"
#include "parsercontext.h"
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

static void _test_pipe_token(void *thread, const Token *token)
{
	fsm_thread_match((FsmThread *)thread, token);
}

static int _test_setup_lexer(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->lexer_thread.handler.target = &context->thread;
	context->lexer_thread.handler.shift = NULL;
	context->lexer_thread.handler.reduce = NULL;
	context->lexer_thread.handler.accept = _test_pipe_token;
	return 0;
}

static int _test_setup_fsm(void *object, void *params)
{
	ParserContext *context = (ParserContext *)object;

	context->thread.handler.target = context->ast;
	context->thread.handler.shift = ast_open;
	context->thread.handler.reduce = ast_close;
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
	listener_init(&fix.parser.parse_end, _test_parse_end, NULL);
	listener_init(&fix.parser.parse_error, _test_parse_error, NULL);
}

void t_teardown(){
	parser_dispose(&fix.parser);
}

void parser_basic_parse(){

	ParserContext context;
	//Input *input;

	parser_context_init(&context, &fix.parser);

	//TODO: write proper test when easier to mockup inputs
	//parser_context_set_input(&context, &input);

	parser_context_dispose(&context);
}

int main(int argc, char** argv){
	t_init(&argc, &argv, NULL);
	t_test(parser_basic_parse);
	return t_done();
}
