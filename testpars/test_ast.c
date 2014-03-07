#include <stddef.h>
#include <string.h>
#include <glib.h>
#include <stdio.h>

#include "ast.h"

typedef struct {
	Ast ast;
} Fixture;

void setup(Fixture *fix, gconstpointer data){
	ast_init(&fix->ast);
}

void teardown(Fixture *fix, gconstpointer data){
	ast_dispose(&fix->ast);
}

void ast_empty(Fixture *fix, gconstpointer data){
	AstCursor cursor;
	AstNode *node;

	ast_cursor_init(&cursor, &fix->ast);
	node = ast_cursor_depth_next(&cursor);
	g_assert(node != NULL);
	node = ast_cursor_depth_next(&cursor);
	g_assert(node == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_single_node(Fixture *fix, gconstpointer data){
	AstCursor cursor;
	AstNode *node;

	ast_open(&fix->ast, 1);
	ast_close(&fix->ast, 3, 3, 123);
	ast_done(&fix->ast);

	ast_cursor_init(&cursor, &fix->ast);
	node = ast_cursor_depth_next(&cursor);
	node = ast_cursor_depth_next(&cursor);

	g_assert(node != NULL);
	g_assert_cmpint(node->index, ==, 1);
	g_assert_cmpint(node->length, ==, 3);
	g_assert_cmpint(node->symbol, ==, 123);

	ast_cursor_dispose(&cursor);
}

int main(int argc, char** argv){
	g_test_init(&argc, &argv, NULL);
	g_test_add("/Ast/match", Fixture, NULL, setup, ast_empty, teardown);
	g_test_add("/Ast/match", Fixture, NULL, setup, ast_single_node, teardown);
	return g_test_run();
}

