#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "input.h"
#include "ast.h"
#include "test.h"

char *buffer = "this is a test";

typedef struct {
	SymbolTable table;
	Input input;
	Ast ast;
} Fixture;

Fixture fix;

void t_setup(){
	input_init_buffer(&fix.input, (unsigned char*)buffer, strlen(buffer));
	symbol_table_init(&fix.table);
	ast_init(&fix.ast, &fix.input, &fix.table);
}

void t_teardown(){
	ast_dispose(&fix.ast);
	symbol_table_dispose(&fix.table);
	input_dispose(&fix.input);
}

void ast_empty(){
	AstCursor cursor;
	AstNode *node;

	ast_cursor_init(&cursor, &fix.ast);
	node = ast_cursor_depth_next(&cursor);
	t_assert(node != NULL);
	node = ast_cursor_depth_next(&cursor);
	t_assert(node == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_single_node(){
	AstCursor cursor;
	AstNode *node;

	ast_open(&fix.ast, 1, 1, 0);
	ast_close(&fix.ast, 3, 3, 123);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	node = ast_cursor_depth_next(&cursor);
	node = ast_cursor_depth_next(&cursor);

	t_assert(node != NULL);
	t_assert(node->index == 1);
	t_assert(node->length == 3);
	t_assert(node->symbol == 123);

	ast_cursor_dispose(&cursor);
}

void ast_nested_nodes(){
	AstCursor cursor;
	AstNode *root, *outer_node;
	AstNode *first_child, *second_child, *grand_child, *last;

	ast_open(&fix.ast, 1, 1, 0);
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123);
	ast_close(&fix.ast, 4, 3, 456);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	root = ast_cursor_depth_next(&cursor);
	outer_node = ast_cursor_depth_next(&cursor);
	first_child = ast_cursor_depth_next(&cursor);
	second_child = ast_cursor_depth_next(&cursor);
	grand_child = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(root != NULL);

	//First opened symbol
	t_assert(outer_node != NULL);
	t_assert(outer_node->index == 1);
	t_assert(outer_node->length == 3);
	t_assert(outer_node->symbol == 456);

	//First opened symbol's implicit shift
	t_assert(first_child != NULL);
	t_assert(first_child->index == 1);
	t_assert(first_child->length == 1);
	t_assert(first_child->symbol == 0);

	//Second opened symbol
	t_assert(second_child != NULL);
	t_assert(second_child->index == 2);
	t_assert(second_child->length == 1);
	t_assert(second_child->symbol == 123);

	//Second opened symbol's implicit shift
	t_assert(grand_child != NULL);
	t_assert(grand_child->index == 2);
	t_assert(grand_child->length == 1);
	t_assert(grand_child->symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_sibling_nodes(){
	AstCursor cursor;
	AstNode *root, *outer, *outer_child, *sibling1, *sibling2;
	AstNode *sibling1_child, *sibling2_child, *last;

	ast_open(&fix.ast, 1, 1, 0);
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123);
	ast_open(&fix.ast, 4, 1, 0);
	ast_close(&fix.ast, 5, 1, 456);
	ast_close(&fix.ast, 6, 5, 789);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	root = ast_cursor_depth_next(&cursor);
	outer = ast_cursor_depth_next(&cursor);
	outer_child = ast_cursor_depth_next(&cursor);
	sibling1 = ast_cursor_depth_next(&cursor);
	sibling1_child = ast_cursor_depth_next(&cursor);
	sibling2 = ast_cursor_depth_next(&cursor);
	sibling2_child = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(root != NULL);

	t_assert(outer != NULL);
	t_assert(outer->index == 1);
	t_assert(outer->length == 5);
	t_assert(outer->symbol == 789);

	t_assert(outer_child != NULL);
	t_assert(outer_child->index == 1);
	t_assert(outer_child->length == 1);
	t_assert(outer_child->symbol == 0);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->index == 2);
	t_assert(sibling1->length == 1);
	t_assert(sibling1->symbol == 123);

	t_assert(sibling1_child != NULL);
	t_assert(sibling1_child->index == 2);
	t_assert(sibling1_child->length == 1);
	t_assert(sibling1_child->symbol == 0);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->index == 4);
	t_assert(sibling2->length == 1);
	t_assert(sibling2->symbol == 456);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->index == 4);
	t_assert(sibling2_child->length == 1);
	t_assert(sibling2_child->symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_same_index_nodes(){
	AstCursor cursor;
	AstNode *root, *outer, *inner, *inner_child, *last;

	ast_open(&fix.ast, 1, 1, 0);
	ast_close(&fix.ast, 4, 3, 123);
	ast_open(&fix.ast, 1, 1, 0);
	ast_close(&fix.ast, 5, 4, 456);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	root = ast_cursor_depth_next(&cursor);
	outer = ast_cursor_depth_next(&cursor);
	inner = ast_cursor_depth_next(&cursor);
	inner_child = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(root != NULL);

	t_assert(outer != NULL);
	t_assert(outer->index == 1);
	t_assert(outer->length == 4);
	t_assert(outer->symbol == 456);

	t_assert(inner!= NULL);
	t_assert(inner->index == 1);
	t_assert(inner->length == 3);
	t_assert(inner->symbol == 123);

	t_assert(inner_child!= NULL);
	t_assert(inner_child->index == 1);
	t_assert(inner_child->length == 1);
	t_assert(inner_child->symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_next_symbol(){
	AstCursor cursor;
	AstNode *sibling1, *sibling2, *sibling2_child, *last;

	ast_open(&fix.ast, 1, 1, 0);
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123);
	ast_open(&fix.ast, 4, 1, 0);
	ast_close(&fix.ast, 5, 1, 123);
	ast_close(&fix.ast, 6, 5, 456);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2_child = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->index == 2);
	t_assert(sibling1->length == 1);
	t_assert(sibling1->symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->index == 4);
	t_assert(sibling2->length == 1);
	t_assert(sibling2->symbol == 123);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->index == 4);
	t_assert(sibling2_child->length == 1);
	t_assert(sibling2_child->symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_next_sibling_symbol(){
	AstCursor cursor;
	AstNode *sibling1, *sibling2, *sibling3, *last;

	ast_open(&fix.ast, 1, 1, 0);
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123); //Inner sibling (should not be raeched)
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123); //First sibling
	ast_open(&fix.ast, 4, 1, 0);
	ast_close(&fix.ast, 5, 1, 123); //Second sibling
	ast_close(&fix.ast, 6, 5, 456);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2 = ast_cursor_next_sibling_symbol(&cursor, 123);
	sibling3 = ast_cursor_next_sibling_symbol(&cursor, 123);
	last = ast_cursor_depth_next(&cursor);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->index == 2);
	t_assert(sibling1->length == 1);
	t_assert(sibling1->symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->index == 4);
	t_assert(sibling2->length == 1);
	t_assert(sibling2->symbol == 123);

	//No sibling after second sibling
	t_assert(sibling3 == NULL);

	t_assert(last != NULL);
	t_assert(last->index == 4);
	t_assert(last->length == 1);
	t_assert(last->symbol == 0);

	ast_cursor_dispose(&cursor);
}

void ast_push_pop_state(){
	AstCursor cursor;
	AstNode *sibling1, *sibling1_child, *sibling2, *sibling2_child;
	AstNode  *sibling2again, *sibling3;

	ast_open(&fix.ast, 1, 1, 0);
	ast_open(&fix.ast, 2, 1, 0);
	ast_close(&fix.ast, 3, 1, 123);
	ast_open(&fix.ast, 4, 1, 0);
	ast_close(&fix.ast, 5, 1, 456);
	ast_close(&fix.ast, 6, 5, 789);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);

	//Skip root
	ast_cursor_depth_next(&cursor);

	sibling1 = ast_cursor_depth_next(&cursor);
	sibling1_child = ast_cursor_depth_next(&cursor);
	ast_cursor_push(&cursor);
	sibling2 = ast_cursor_depth_next(&cursor);
	sibling2_child = ast_cursor_depth_next(&cursor);
	sibling3 = ast_cursor_depth_next(&cursor);
	ast_cursor_pop(&cursor);

	sibling2again = ast_cursor_depth_next(&cursor);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->index == 1);
	t_assert(sibling1->length == 5);
	t_assert(sibling1->symbol == 789);

	t_assert(sibling1_child != NULL);
	t_assert(sibling1_child->index == 1);
	t_assert(sibling1_child->length == 1);
	t_assert(sibling1_child->symbol == 0);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->index == 2);
	t_assert(sibling2->length == 1);
	t_assert(sibling2->symbol == 123);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->index == 2);
	t_assert(sibling2_child->length == 1);
	t_assert(sibling2_child->symbol == 0);

	t_assert(sibling3 != NULL);
	t_assert(sibling3->index == 4);
	t_assert(sibling3->length == 1);
	t_assert(sibling3->symbol == 456);

	t_assert(sibling2again != NULL);
	t_assert(sibling2again == sibling2);
	t_assert(sibling2again->index == 2);
	t_assert(sibling2again->length == 1);
	t_assert(sibling2again->symbol == 123);

	ast_cursor_dispose(&cursor);
}

void ast_cursor_get_strings(){
	AstCursor cursor;
	unsigned char *string;
	int length, diff;

	ast_open(&fix.ast, 0, 1, 0); // this is a test
	ast_open(&fix.ast, 5, 1, 0); // is
	ast_close(&fix.ast, 6, 2, 123);
	ast_open(&fix.ast, 8, 1, 0); // a
	ast_close(&fix.ast, 8, 1, 456);
	ast_close(&fix.ast, 13, 14, 789);
	ast_done(&fix.ast);

	ast_cursor_init(&cursor, &fix.ast);

	//Skip root
	ast_cursor_depth_next(&cursor);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "this is a test", strlen("this is a test"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "t", strlen("t"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "is", strlen("is"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "i", strlen("i"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "a", strlen("a"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp((char*)string, "a", strlen("a"));
	t_assert(diff == 0);

	ast_cursor_dispose(&cursor);
}

int main(int argc, char** argv){
	t_init();
	t_test(ast_empty);
	t_test(ast_single_node);
	t_test(ast_nested_nodes);
	t_test(ast_sibling_nodes);
	t_test(ast_same_index_nodes);
	t_test(ast_next_symbol);
	t_test(ast_next_sibling_symbol);
	t_test(ast_push_pop_state);
	t_test(ast_cursor_get_strings);
	return t_done();
}

