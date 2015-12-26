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
	char *in1="BLUE", *out1;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), in1);
	out1 = radix_tree_get(tree, nzs("blue"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert_cmpstr(out1, ==, "BLUE");
}

void test_radix_tree__set_and_get_1key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *out1;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), in1);
	out1 = radix_tree_get(tree, nzs("b"));

	g_assert(tree->type == NODE_TYPE_TREE);
	g_assert(tree->size == 1);
	g_assert_cmpstr(out1, ==, "BLUE");
}

void test_radix_tree__set_and_remove_1key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *out1;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), in1);
	radix_tree_remove(tree, nzs("b"));
	out1 = radix_tree_get(tree, nzs("b"));

	g_assert(tree->type == NODE_TYPE_LEAF);
	g_assert(tree->size == 0);
	g_assert(out1 == NULL);
}

void test_radix_tree__set2_and_remove1_1key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("b"), in1);
	radix_tree_set(tree, nzs("g"), in2);
	radix_tree_remove(tree, nzs("b"));
	out1 = radix_tree_get(tree, nzs("b"));
	out2 = radix_tree_get(tree, nzs("g"));

	g_assert(tree->type == NODE_TYPE_TREE);
	g_assert(tree->size == 1);
	g_assert(out1 == NULL);
	g_assert_cmpstr(out2, ==, "GREEN");
}

void test_radix_tree__set_and_remove_4key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *out1;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), in1);
	radix_tree_remove(tree, nzs("blue"));
	out1= radix_tree_get(tree, nzs("blue"));

	g_assert(tree->type == NODE_TYPE_LEAF);
	g_assert(tree->size == 0);
	g_assert(out1 == NULL);
}

void test_radix_tree__set2_and_remove1_4key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("blue"), in1);
	radix_tree_set(tree, nzs("green"), in2);
	radix_tree_remove(tree, nzs("blue"));
	out1 = radix_tree_get(tree, nzs("blue"));
	out2 = radix_tree_get(tree, nzs("green"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 5);
	g_assert(out1 == NULL);
	g_assert_cmpstr(out2, ==, "GREEN");
}

void test_radix_tree__set2_and_remove1_4key_with_parent_array(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lightblue"), in1);
	radix_tree_set(tree, nzs("lightgreen"), in2);
	radix_tree_remove(tree, nzs("lightblue"));
	out1 = radix_tree_get(tree, nzs("lightblue"));
	out2 = radix_tree_get(tree, nzs("lightgreen"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 10);
	g_assert(out1 == NULL);
	g_assert_cmpstr(out2, ==, "GREEN");
}

void test_radix_tree__set2_and_remove1_4key_deep(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *in3="LIG", *out1, *out2, *out6;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lightblue"), in1);
	radix_tree_set(tree, nzs("lightgreen"), in2);
	radix_tree_set(tree, nzs("lig"), in3);
	radix_tree_remove(tree, nzs("lightblue"));
	out1 = radix_tree_get(tree, nzs("lightblue"));
	out2 = radix_tree_get(tree, nzs("lightgreen"));
	out6 = radix_tree_get(tree, nzs("lig"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 3);
	g_assert(out1== NULL);
	g_assert_cmpstr(out2, ==, "GREEN");
	g_assert_cmpstr(out6, ==, "LIG");
}

void test_radix_tree__set2_and_remove1_3key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lbl"), in1);
	radix_tree_set(tree, nzs("lgr"), in2);
	radix_tree_remove(tree, nzs("lbl"));
	out1 = radix_tree_get(tree, nzs("lbl"));
	out2 = radix_tree_get(tree, nzs("lgr"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 3);
	g_assert(out1 == NULL);
	g_assert_cmpstr(out2, ==, "GREEN");
}

void test_radix_tree__remove_non_leaf_key(RadixTreeFixture* fix, gconstpointer data){
	char *in1="LIGHTGREEN", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;

	radix_tree_set(tree, nzs("lightgreen"), in1);
	radix_tree_set(tree, nzs("light"), in2);
	radix_tree_remove(tree, nzs("light"));
	out1 = radix_tree_get(tree, nzs("lightgreen"));
	out2 = radix_tree_get(tree, nzs("light"));

	g_assert(tree->type == NODE_TYPE_ARRAY);
	g_assert(tree->size == 10);
	g_assert_cmpstr(out1, ==, "LIGHTGREEN");
	g_assert(out2 == NULL);
}

void test_radix_tree__non_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="GREEN", *out1, *out2;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("blue"), in1);
	radix_tree_set(tree, nzs("green"), in2);
	out1 = radix_tree_get(tree, nzs("blue"));
	out2 = radix_tree_get(tree, nzs("green"));
	g_assert_cmpstr(out1, ==, "BLUE");
	g_assert_cmpstr(out2, ==, "GREEN");
}

void test_radix_tree__first_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BLUE", *in2="BOBO", *out1, *out2;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("blue"), in1);
	radix_tree_set(tree, nzs("bobo"), in2);
	out1 = radix_tree_get(tree, nzs("blue"));
	out2 = radix_tree_get(tree, nzs("bobo"));
	g_assert_cmpstr(out1, ==, "BLUE");
	g_assert_cmpstr(out2, ==, "BOBO");
}

void test_radix_tree__last_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
	char *in1="BOBI", *in2="BOBO", *out1, *out2;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("bobi"), in1);
	radix_tree_set(tree, nzs("bobo"), in2);
	out1 = radix_tree_get(tree, nzs("bobi"));
	g_assert_cmpstr(out1, ==, "BOBI");
	out2 = radix_tree_get(tree, nzs("bobo"));
	g_assert_cmpstr(out2, ==, "BOBO");
}

void test_radix_tree__add_prefix(RadixTreeFixture* fix, gconstpointer data){
	char *in1="DINOSAURIO", *in2="DINO", *out1, *out2;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("dinosaurio"), in1);
	radix_tree_set(tree, nzs("dino"), in2);
	out1 = radix_tree_get(tree, nzs("dinosaurio"));
	out2 = radix_tree_get(tree, nzs("dino"));
	g_assert_cmpstr(out1, ==, "DINOSAURIO");
	g_assert_cmpstr(out2, ==, "DINO");
}

void test_radix_tree__add_suffix(RadixTreeFixture* fix, gconstpointer data){
	char *in1="DINOSAURIO", *in2="DINO", *out1, *out2;
	Node *tree = &fix->tree;
	radix_tree_set(tree, nzs("dino"), in2);
	radix_tree_set(tree, nzs("dinosaurio"), in1);
	out1 = radix_tree_get(tree, nzs("dinosaurio"));
	out2 = radix_tree_get(tree, nzs("dino"));
	g_assert_cmpstr(out1, ==, "DINOSAURIO");
	g_assert_cmpstr(out2, ==, "DINO");
}

void test_radix_tree__iterate(RadixTreeFixture* fix, gconstpointer data){
	char *in1="DINOSAURIO", *in2="DINO", *in3="CASA", *in4="PIANO";
	char *out1, *out2, *out3, *out4, *out5;
	Node *tree = &fix->tree;
	Iterator it;
	radix_tree_set(tree, nzs("dino"), in2);
	radix_tree_set(tree, nzs("piano"), in4);
	radix_tree_set(tree, nzs("dinosaurio"), in1);
	radix_tree_set(tree, nzs("casa"), in3);
	radix_tree_iterator_init(tree, &it);
	out1 = (char *)radix_tree_iterator_next(tree, &it);
	out2 = (char *)radix_tree_iterator_next(tree, &it);
	out3 = (char *)radix_tree_iterator_next(tree, &it);
	out4 = (char *)radix_tree_iterator_next(tree, &it);
	out5 = (char *)radix_tree_iterator_next(tree, &it);
	radix_tree_iterator_dispose(tree, &it);
	g_assert_cmpstr(out1, ==, "CASA");
	g_assert_cmpstr(out2, ==, "DINO");
	g_assert_cmpstr(out3, ==, "DINOSAURIO");
	g_assert_cmpstr(out4, ==, "PIANO");
	g_assert(out5 == NULL);
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
	g_test_add("/RadixTree/set2_and_remove1_4key_deep", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_4key_deep, radix_tree_teardown);
	g_test_add("/RadixTree/set2_and_remove1_3key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set2_and_remove1_3key, radix_tree_teardown);
	g_test_add("/RadixTree/remove_non_leaf_key", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__remove_non_leaf_key, radix_tree_teardown);
	g_test_add("/RadixTree/non_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__non_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/first_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__first_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/last_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__last_clashing_keys, radix_tree_teardown);
	g_test_add("/RadixTree/add_prefix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_prefix, radix_tree_teardown);
	g_test_add("/RadixTree/add_suffix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_suffix, radix_tree_teardown);
	g_test_add("/RadixTree/iterate", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__iterate, radix_tree_teardown);
	return g_test_run();
}

