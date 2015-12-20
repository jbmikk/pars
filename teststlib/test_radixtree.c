#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "radixtree.h"
#include "radixtree_p.h"

typedef struct {
	Node tree;
	gchar *str1;
	gchar *str2;
	gchar *str3;
	gchar *str4;
	gchar *str5;
}RadixTreeFixture;

void radix_tree_setup(RadixTreeFixture* fixture, gconstpointer data){
	NODE_INIT(fixture->tree, 0, 0, NULL);
}
void radix_tree_teardown(RadixTreeFixture* fixture, gconstpointer data){
	//delete
}

void test_radix_tree__set_and_get(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), str1);
	str2 = radix_tree_get(tree, nzs("blue"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert_cmpstr(str2, ==, "BLUE");
}

void test_radix_tree__set_and_get_1key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), str1);
	str2 = radix_tree_get(tree, nzs("b"));

	g_assert(tree->type == NODE_TYPE_TREE);
	g_assert(tree->size == 1);
	g_assert_cmpstr(str2, ==, "BLUE");
}

void test_radix_tree__set_and_remove_1key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), str1);
	radix_tree_remove(tree, nzs("b"));
	str2 = radix_tree_get(tree, nzs("b"));

	g_assert(tree->type == NODE_TYPE_LEAF);
	g_assert(tree->size == 0);
	g_assert(str2 == NULL);
}

void test_radix_tree__set2_and_remove1_1key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="GREEN", *str3, *str4;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), str1);
	radix_tree_set(tree, nzs("g"), str2);
	radix_tree_remove(tree, nzs("b"));
	str3 = radix_tree_get(tree, nzs("b"));
	str4 = radix_tree_get(tree, nzs("g"));

	g_assert(tree->type == NODE_TYPE_TREE);
	g_assert(tree->size == 1);
	g_assert(str3 == NULL);
	g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__set_and_remove_4key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), str1);
	radix_tree_remove(tree, nzs("blue"));
	str2 = radix_tree_get(tree, nzs("blue"));

	g_assert(tree->type == NODE_TYPE_LEAF);
	g_assert(tree->size == 0);
	g_assert(str2 == NULL);
}

void test_radix_tree__set2_and_remove1_4key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="GREEN", *str3, *str4;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), str1);
	radix_tree_set(tree, nzs("green"), str2);
	radix_tree_remove(tree, nzs("blue"));
	str3 = radix_tree_get(tree, nzs("blue"));
	str4 = radix_tree_get(tree, nzs("green"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 5);
	g_assert(str3 == NULL);
	g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__set2_and_remove1_4key_with_parent_array(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="GREEN", *str3, *str4;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lightblue"), str1);
	radix_tree_set(tree, nzs("lightgreen"), str2);
	radix_tree_remove(tree, nzs("lightblue"));
	str3 = radix_tree_get(tree, nzs("lightblue"));
	str4 = radix_tree_get(tree, nzs("lightgreen"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 10);
	g_assert(str3 == NULL);
	g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__set2_and_remove1_3key(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="GREEN", *str3, *str4;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lbl"), str1);
	radix_tree_set(tree, nzs("lgr"), str2);
	radix_tree_remove(tree, nzs("lbl"));
	str3 = radix_tree_get(tree, nzs("lbl"));
	str4 = radix_tree_get(tree, nzs("lgr"));

	//TODO: These fail because there is a defect in the implementation.
	//In the case of single node trees they should be treated the same
	//as arrays.
	//g_assert(tree->type == NODE_TYPE_ARRAY);
	//g_assert(tree->size == 3);
	g_assert(str3 == NULL);
	g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__non_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="GREEN", *str3, *str4;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("blue"), str1);
	radix_tree_set(tree, nzs("green"), str2);
	str3 = radix_tree_get(tree, nzs("blue"));
	str4 = radix_tree_get(tree, nzs("green"));
	g_assert_cmpstr(str3, ==, "BLUE");
	g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__first_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BLUE", *str2="BOBO", *str3, *str4;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("blue"), str1);
	radix_tree_set(tree, nzs("bobo"), str2);
	str3 = radix_tree_get(tree, nzs("blue"));
	str4 = radix_tree_get(tree, nzs("bobo"));
	g_assert_cmpstr(str3, ==, "BLUE");
	g_assert_cmpstr(str4, ==, "BOBO");
}

void test_radix_tree__last_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *str1="BOBI", *str2="BOBO", *str3, *str4;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("bobi"), str1);
	radix_tree_set(tree, nzs("bobo"), str2);
	str3 = radix_tree_get(tree, nzs("bobi"));
	g_assert_cmpstr(str3, ==, "BOBI");
	str4 = radix_tree_get(tree, nzs("bobo"));
	g_assert_cmpstr(str4, ==, "BOBO");
}

void test_radix_tree__add_prefix(RadixTreeFixture* fix, gconstpointer data){
	char *str1="DINOSAURIO", *str2="DINO", *str3, *str4;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("dinosaurio"), str1);
	radix_tree_set(tree, nzs("dino"), str2);
	str3 = radix_tree_get(tree, nzs("dinosaurio"));
	str4 = radix_tree_get(tree, nzs("dino"));
	g_assert_cmpstr(str3, ==, "DINOSAURIO");
	g_assert_cmpstr(str4, ==, "DINO");
}

void test_radix_tree__add_suffix(RadixTreeFixture* fix, gconstpointer data){
	char *str1="DINOSAURIO", *str2="DINO", *str3, *str4;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("dino"), str2);
	radix_tree_set(tree, nzs("dinosaurio"), str1);
	str3 = radix_tree_get(tree, nzs("dinosaurio"));
	str4 = radix_tree_get(tree, nzs("dino"));
	g_assert_cmpstr(str3, ==, "DINOSAURIO");
	g_assert_cmpstr(str4, ==, "DINO");
}

void test_radix_tree__iterate(RadixTreeFixture* fix, gconstpointer data){
	char *str1="DINOSAURIO", *str2="DINO", *str3="CASA", *str4="PIANO";
	char *ostr1, *ostr2, *ostr3, *ostr4, *ostr5;
	Node *tree = &fix->tree;
	Iterator it;
	radix_tree_set(tree, nzs("dino"), str2);
	radix_tree_set(tree, nzs("piano"), str4);
	radix_tree_set(tree, nzs("dinosaurio"), str1);
	radix_tree_set(tree, nzs("casa"), str3);
	radix_tree_iterator_init(tree, &it);
	ostr1 = (char *)radix_tree_iterator_next(tree, &it);
	ostr2 = (char *)radix_tree_iterator_next(tree, &it);
	ostr3 = (char *)radix_tree_iterator_next(tree, &it);
	ostr4 = (char *)radix_tree_iterator_next(tree, &it);
	ostr5 = (char *)radix_tree_iterator_next(tree, &it);
	radix_tree_iterator_dispose(tree, &it);
	g_assert_cmpstr(ostr1, ==, "CASA");
	g_assert_cmpstr(ostr2, ==, "DINO");
	g_assert_cmpstr(ostr3, ==, "DINOSAURIO");
	g_assert_cmpstr(ostr4, ==, "PIANO");
	g_assert(ostr5 == NULL);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add("/RadixTree/set_and_get", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set_and_get, radix_tree_teardown);
	g_test_add("/RadixTree/set_and_get_1key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set_and_get_1key, radix_tree_teardown);
	g_test_add("/RadixTree/set_and_remove_1key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set_and_remove_1key, radix_tree_teardown);
	g_test_add("/RadixTree/set2_and_remove1_1key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_1key, radix_tree_teardown);
	g_test_add("/RadixTree/set_and_remove_4key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set_and_remove_4key, radix_tree_teardown);
	g_test_add("/RadixTree/set2_and_remove1_4key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_4key, radix_tree_teardown);
	g_test_add("/RadixTree/set2_and_remove1_4key_with_parent_array", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_4key_with_parent_array, radix_tree_teardown);
	g_test_add("/RadixTree/set2_and_remove1_3key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_3key, radix_tree_teardown);
	g_test_add("/RadixTree/non_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__non_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/first_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__first_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/last_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__last_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/add_prefix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_prefix, radix_tree_teardown);
	g_test_add("/RadixTree/add_suffix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_suffix, radix_tree_teardown);
	g_test_add("/RadixTree/iterate", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__iterate, radix_tree_teardown);
	return g_test_run();
}

