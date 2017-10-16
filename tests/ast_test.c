#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "input.h"
#include "ast.h"
#include "astbuilder.h"
#include "test.h"

char *buffer = "this is a test";

typedef struct {
	SymbolTable table;
	Input input;
	Ast ast;
} Fixture;

Fixture fix;

void t_setup(){
	input_set_data(&fix.input, buffer, strlen(buffer));
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
	t_assert(cursor.offset == 0);
	node = ast_cursor_depth_next(&cursor);
	t_assert(node == NULL);
	t_assert(cursor.offset == 0);

	ast_cursor_dispose(&cursor);
}

void ast_single_node(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *node;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 3, 123, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	node = ast_cursor_depth_next(&cursor);
	node = ast_cursor_depth_next(&cursor);

	t_assert(node != NULL);
	t_assert(node->token.index == 1);
	t_assert(node->token.length == 3);
	t_assert(node->token.symbol == 123);
	t_assert(cursor.offset == 1);

	ast_cursor_dispose(&cursor);
}

void ast_nested_nodes(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *root, *outer_node;
	AstNode *first_child, *second_child, *grand_child, *last;
	int root_offset, outer_node_offset, first_child_offset,
		second_child_offset, grand_child_offset, last_offset;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_reduce(&builder, &(Token){4, 3, 456, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	root = ast_cursor_depth_next(&cursor);
	root_offset = cursor.offset;
	outer_node = ast_cursor_depth_next(&cursor);
	outer_node_offset = cursor.offset;
	first_child = ast_cursor_depth_next(&cursor);
	first_child_offset = cursor.offset;
	second_child = ast_cursor_depth_next(&cursor);
	second_child_offset = cursor.offset;
	grand_child = ast_cursor_depth_next(&cursor);
	grand_child_offset = cursor.offset;
	last = ast_cursor_depth_next(&cursor);
	last_offset = cursor.offset;

	t_assert(root != NULL);
	t_assert(root_offset == 0);

	//First opened symbol
	t_assert(outer_node != NULL);
	t_assert(outer_node->token.index == 1);
	t_assert(outer_node->token.length == 3);
	t_assert(outer_node->token.symbol == 456);
	t_assert(outer_node_offset == 1);

	//First opened symbol's implicit shift
	t_assert(first_child != NULL);
	t_assert(first_child->token.index == 1);
	t_assert(first_child->token.length == 1);
	t_assert(first_child->token.symbol == 0);
	t_assert(first_child_offset == 1);

	//Second opened symbol
	t_assert(second_child != NULL);
	t_assert(second_child->token.index == 2);
	t_assert(second_child->token.length == 1);
	t_assert(second_child->token.symbol == 123);
	t_assert(second_child_offset == 0);

	//Second opened symbol's implicit shift
	t_assert(grand_child != NULL);
	t_assert(grand_child->token.index == 2);
	t_assert(grand_child->token.length == 1);
	t_assert(grand_child->token.symbol == 0);
	t_assert(grand_child_offset == 1);

	t_assert(last == NULL);
	t_assert(last_offset == -3);

	ast_cursor_dispose(&cursor);
}

void ast_sibling_nodes(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *root, *outer, *outer_child, *sibling1, *sibling2;
	AstNode *sibling1_child, *sibling2_child, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 456, 0});
	ast_builder_reduce(&builder, &(Token){6, 5, 789, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

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
	t_assert(outer->token.index == 1);
	t_assert(outer->token.length == 5);
	t_assert(outer->token.symbol == 789);

	t_assert(outer_child != NULL);
	t_assert(outer_child->token.index == 1);
	t_assert(outer_child->token.length == 1);
	t_assert(outer_child->token.symbol == 0);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 2);
	t_assert(sibling1->token.length == 1);
	t_assert(sibling1->token.symbol == 123);

	t_assert(sibling1_child != NULL);
	t_assert(sibling1_child->token.index == 2);
	t_assert(sibling1_child->token.length == 1);
	t_assert(sibling1_child->token.symbol == 0);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 4);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 456);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->token.index == 4);
	t_assert(sibling2_child->token.length == 1);
	t_assert(sibling2_child->token.symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_same_index_nodes(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *root, *outer, *inner, *inner_child, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){4, 3, 123, 0});
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 4, 456, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	root = ast_cursor_depth_next(&cursor);
	outer = ast_cursor_depth_next(&cursor);
	inner = ast_cursor_depth_next(&cursor);
	inner_child = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(root != NULL);

	t_assert(outer != NULL);
	t_assert(outer->token.index == 1);
	t_assert(outer->token.length == 4);
	t_assert(outer->token.symbol == 456);

	t_assert(inner!= NULL);
	t_assert(inner->token.index == 1);
	t_assert(inner->token.length == 3);
	t_assert(inner->token.symbol == 123);

	t_assert(inner_child!= NULL);
	t_assert(inner_child->token.index == 1);
	t_assert(inner_child->token.length == 1);
	t_assert(inner_child->token.symbol == 0);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_symbol_descendant_next(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *sibling1, *sibling2, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 123, 0});
	ast_builder_reduce(&builder, &(Token){6, 5, 456, 0});
	ast_builder_shift(&builder, &(Token){7, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){8, 1, 123, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_descendant_next_symbol(&cursor, 456);
	sibling2 = ast_cursor_descendant_next_symbol(&cursor, 123);
	last = ast_cursor_descendant_next_symbol(&cursor, 123);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 1);
	t_assert(sibling1->token.length == 5);
	t_assert(sibling1->token.symbol == 456);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 2);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_next_symbol(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *sibling1, *sibling2, *sibling3, *sibling2_child, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 123, 0});
	ast_builder_reduce(&builder, &(Token){6, 5, 456, 0});
	ast_builder_shift(&builder, &(Token){7, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){8, 1, 123, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2_child = ast_cursor_depth_next(&cursor);
	sibling3 = ast_cursor_depth_next_symbol(&cursor, 123);
	last = ast_cursor_depth_next(&cursor);
	last = ast_cursor_depth_next(&cursor);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 2);
	t_assert(sibling1->token.length == 1);
	t_assert(sibling1->token.symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 4);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->token.index == 4);
	t_assert(sibling2_child->token.length == 1);
	t_assert(sibling2_child->token.symbol == 0);

	t_assert(sibling3 != NULL);
	t_assert(sibling3->token.index == 7);
	t_assert(sibling3->token.length == 1);
	t_assert(sibling3->token.symbol == 123);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_relative_symbol_next(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *sibling1, *sibling2, *sibling3, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 123, 0});
	ast_builder_reduce(&builder, &(Token){6, 5, 456, 0});
	ast_builder_shift(&builder, &(Token){7, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){8, 1, 123, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_descendant_next_symbol(&cursor, 456);
	ast_cursor_push(&cursor);
	sibling2 = ast_cursor_descendant_next_symbol(&cursor, 123);
	sibling3 = ast_cursor_relative_next_symbol(&cursor, 123);
	last = ast_cursor_relative_next_symbol(&cursor, 123);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 1);
	t_assert(sibling1->token.length == 5);
	t_assert(sibling1->token.symbol == 456);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 2);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(sibling3 != NULL);
	t_assert(sibling3->token.index == 4);
	t_assert(sibling3->token.length == 1);
	t_assert(sibling3->token.symbol == 123);

	t_assert(last == NULL);

	ast_cursor_dispose(&cursor);
}

void ast_next_sibling_symbol(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *sibling1, *sibling2, *sibling3, *last;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0}); //Inner sibling (should not be raeched)
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0}); //First sibling
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 456, 0}); //Second sibling (should be skipped)
	ast_builder_shift(&builder, &(Token){6, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){7, 1, 123, 0}); //Third sibling
	ast_builder_reduce(&builder, &(Token){8, 5, 456, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);
	sibling1 = ast_cursor_depth_next_symbol(&cursor, 123);
	sibling2 = ast_cursor_next_sibling_symbol(&cursor, 123);
	sibling3 = ast_cursor_next_sibling_symbol(&cursor, 123);
	last = ast_cursor_depth_next(&cursor);

	t_assert(sibling1 != NULL);
	t_assert(sibling1->token.index == 2);
	t_assert(sibling1->token.length == 1);
	t_assert(sibling1->token.symbol == 123);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 6);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	//No sibling after second sibling
	t_assert(sibling3 == NULL);

	t_assert(last != NULL);
	t_assert(last->token.index == 6);
	t_assert(last->token.length == 1);
	t_assert(last->token.symbol == 0);

	ast_cursor_dispose(&cursor);
}

void ast_push_pop_state(){
	AstCursor cursor;
	AstBuilder builder;
	AstNode *sibling1, *sibling1_child, *sibling2, *sibling2_child;
	AstNode  *sibling2again, *sibling3;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){1, 1, 0, 0});
	ast_builder_shift(&builder, &(Token){2, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){3, 1, 123, 0});
	ast_builder_shift(&builder, &(Token){4, 1, 0, 0});
	ast_builder_reduce(&builder, &(Token){5, 1, 456, 0});
	ast_builder_reduce(&builder, &(Token){6, 5, 789, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

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
	t_assert(sibling1->token.index == 1);
	t_assert(sibling1->token.length == 5);
	t_assert(sibling1->token.symbol == 789);

	t_assert(sibling1_child != NULL);
	t_assert(sibling1_child->token.index == 1);
	t_assert(sibling1_child->token.length == 1);
	t_assert(sibling1_child->token.symbol == 0);

	t_assert(sibling2 != NULL);
	t_assert(sibling2->token.index == 2);
	t_assert(sibling2->token.length == 1);
	t_assert(sibling2->token.symbol == 123);

	t_assert(sibling2_child != NULL);
	t_assert(sibling2_child->token.index == 2);
	t_assert(sibling2_child->token.length == 1);
	t_assert(sibling2_child->token.symbol == 0);

	t_assert(sibling3 != NULL);
	t_assert(sibling3->token.index == 4);
	t_assert(sibling3->token.length == 1);
	t_assert(sibling3->token.symbol == 456);

	t_assert(sibling2again != NULL);
	t_assert(sibling2again == sibling2);
	t_assert(sibling2again->token.index == 2);
	t_assert(sibling2again->token.length == 1);
	t_assert(sibling2again->token.symbol == 123);

	ast_cursor_dispose(&cursor);
}

void ast_cursor_get_strings(){
	AstCursor cursor;
	AstBuilder builder;
	char *string;
	int length, diff;

	ast_builder_init(&builder, &fix.ast);
	ast_builder_shift(&builder, &(Token){0, 1, 0, 0}); // this is a test
	ast_builder_shift(&builder, &(Token){5, 1, 0, 0}); // is
	ast_builder_reduce(&builder, &(Token){6, 2, 123, 0});
	ast_builder_shift(&builder, &(Token){8, 1, 0, 0}); // a
	ast_builder_reduce(&builder, &(Token){8, 1, 456, 0});
	ast_builder_reduce(&builder, &(Token){13, 14, 789, 0});
	ast_builder_done(&builder);
	ast_builder_dispose(&builder);

	ast_cursor_init(&cursor, &fix.ast);

	//Skip root
	ast_cursor_depth_next(&cursor);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "this is a test", strlen("this is a test"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "t", strlen("t"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "is", strlen("is"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "i", strlen("i"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "a", strlen("a"));
	t_assert(diff == 0);

	ast_cursor_depth_next(&cursor);
	ast_cursor_get_string(&cursor, &string, &length);
	diff = strncmp(string, "a", strlen("a"));
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
	t_test(ast_symbol_descendant_next);
	t_test(ast_next_symbol);
	t_test(ast_next_sibling_symbol);
	t_test(ast_relative_symbol_next);
	t_test(ast_push_pop_state);
	t_test(ast_cursor_get_strings);
	return t_done();
}

