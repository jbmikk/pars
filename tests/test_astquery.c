#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "input.h"
#include "ast.h"
#include "astquery.h"
#include "test.h"

char *buffer = "this is a test";

typedef struct {
	SymbolTable table;
	Input input;
	Ast ast;
} Fixture;

Fixture fix;

void t_setup(){
	input_init_buffer(&fix.input, buffer, strlen(buffer));
	symbol_table_init(&fix.table);
	ast_init(&fix.ast, &fix.input, &fix.table);
}

void t_teardown(){
	ast_dispose(&fix.ast);
	symbol_table_dispose(&fix.table);
	input_dispose(&fix.input);
}

void ast_query_sibling_iteration(){
	AstQuery query;
	AstNode *sibling1, *sibling2, *sibling3, *last;

	ast_open(&fix.ast, &(Token){1, 1, 0});
	ast_open(&fix.ast, &(Token){2, 1, 0});
	ast_close(&fix.ast, &(Token){3, 1, 123});
	ast_open(&fix.ast, &(Token){4, 1, 0});
	ast_close(&fix.ast, &(Token){5, 1, 123});
	ast_close(&fix.ast, &(Token){6, 5, 456});
	ast_open(&fix.ast, &(Token){7, 1, 0});
	ast_close(&fix.ast, &(Token){8, 1, 123});
	ast_done(&fix.ast);

	int symbols[1] = { 123 };
	ast_query_init(&query, &fix.ast, symbols, 1);
	sibling1 = ast_query_next(&query);
	sibling2 = ast_query_next(&query);
	sibling3 = ast_query_next(&query);
	last = ast_query_next(&query);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 2);
	t_assert(sibling1->token.length == 1);
	t_assert(sibling1->token.symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 4);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(sibling3 != NULL);
	t_assert(sibling3->token.index == 7);
	t_assert(sibling3->token.length == 1);
	t_assert(sibling3->token.symbol == 123);

	t_assert(last == NULL);

	ast_query_dispose(&query);
}

void ast_query_nested_iteration(){
	AstQuery query;
	AstNode *sibling1, *sibling2, *last;

	ast_open(&fix.ast, &(Token){1, 1, 0});
	ast_close(&fix.ast, &(Token){6, 5, 456});
	ast_open(&fix.ast, &(Token){7, 1, 0});
	ast_open(&fix.ast, &(Token){8, 1, 0});
	ast_close(&fix.ast, &(Token){9, 1, 123});
	ast_close(&fix.ast, &(Token){10, 1, 123});
	ast_done(&fix.ast);

	int symbols[1] = { 123 };
	ast_query_init(&query, &fix.ast, symbols, 1);
	sibling1 = ast_query_next(&query);
	sibling2 = ast_query_next(&query);
	last = ast_query_next(&query);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 7);
	t_assert(sibling1->token.length == 1);
	t_assert(sibling1->token.symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 8);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(last == NULL);

	ast_query_dispose(&query);
}
int main(int argc, char** argv){
	t_init();
	t_test(ast_query_sibling_iteration);
	t_test(ast_query_nested_iteration);
	return t_done();
}

