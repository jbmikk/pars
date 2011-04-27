#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "cradixtree.h"

typedef struct {
    CNode tree;
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
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "blue", 4, str1);
    str2 = c_radix_tree_get(tree, "blue", 4);
    g_assert_cmpstr(str2, ==, "BLUE");
}

void test_radix_tree__non_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
    char *str1="BLUE", *str2="GREEN", *str3, *str4;
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "blue", 4, str1);
    c_radix_tree_set(tree, "green", 5, str2);
    str3 = c_radix_tree_get(tree, "blue", 4);
    str4 = c_radix_tree_get(tree, "green", 5);
    g_assert_cmpstr(str3, ==, "BLUE");
    g_assert_cmpstr(str4, ==, "GREEN");
}

void test_radix_tree__first_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
    char *str1="BLUE", *str2="BOBO", *str3, *str4;
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "blue", 4, str1);
    c_radix_tree_set(tree, "bobo", 4, str2);
    str3 = c_radix_tree_get(tree, "blue", 4);
    str4 = c_radix_tree_get(tree, "bobo", 4);
    g_assert_cmpstr(str3, ==, "BLUE");
    g_assert_cmpstr(str4, ==, "BOBO");
}

void test_radix_tree__last_clashing_keys(RadixTreeFixture* fix, gconstpointer data){
    char *str1="BOBI", *str2="BOBO", *str3, *str4;
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "bobi", 4, str1);
    c_radix_tree_set(tree, "bobo", 4, str2);
    str3 = c_radix_tree_get(tree, "bobi", 4);
    g_assert_cmpstr(str3, ==, "BOBI");
    str4 = c_radix_tree_get(tree, "bobo", 4);
    g_assert_cmpstr(str4, ==, "BOBO");
}

void test_radix_tree__add_prefix(RadixTreeFixture* fix, gconstpointer data){
    char *str1="DINOSAURIO", *str2="DINO", *str3, *str4;
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "dinosaurio", 10, str1);
    c_radix_tree_set(tree, "dino", 4, str2);
    str3 = c_radix_tree_get(tree, "dinosaurio", 10);
    str4 = c_radix_tree_get(tree, "dino", 4);
    g_assert_cmpstr(str3, ==, "DINOSAURIO");
    g_assert_cmpstr(str4, ==, "DINO");
}

void test_radix_tree__add_suffix(RadixTreeFixture* fix, gconstpointer data){
    char *str1="DINOSAURIO", *str2="DINO", *str3, *str4;
    CNode *tree = &fix->tree;
    c_radix_tree_set(tree, "dino", 4, str2);
    c_radix_tree_set(tree, "dinosaurio", 10, str1);
    str3 = c_radix_tree_get(tree, "dinosaurio", 10);
    str4 = c_radix_tree_get(tree, "dino", 4);
    g_assert_cmpstr(str3, ==, "DINOSAURIO");
    g_assert_cmpstr(str4, ==, "DINO");
}

int main(int argc, char** argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add("/RadixTree/set_and_get", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__set_and_get, radix_tree_teardown);
    g_test_add("/RadixTree/non_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__non_clashing_keys, radix_tree_teardown);
    g_test_add("/RadixTree/first_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__first_clashing_keys, radix_tree_teardown);
    g_test_add("/RadixTree/last_clashing_keys", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__last_clashing_keys, radix_tree_teardown);
    g_test_add("/RadixTree/add_prefix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_prefix, radix_tree_teardown);
    g_test_add("/RadixTree/add_suffix", RadixTreeFixture, NULL, radix_tree_setup, test_radix_tree__add_suffix, radix_tree_teardown);
    return g_test_run();
}

