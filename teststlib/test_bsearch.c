#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "bsearch.h"

typedef struct {
	Node node;
	gchar *str1;
	gchar *str2;
	gchar *str3;
	gchar *str4;
	gchar *str5;
}BSearchFixture;

void bsearch_setup(BSearchFixture* fixture, gconstpointer data){
	NODE_INIT(fixture->node, 0, 0, NULL);
}
void bsearch_teardown(BSearchFixture* fixture, gconstpointer data){
	bsearch_delete_all(&fixture->node);
}

void bsearch__set_and_get(BSearchFixture* fix, gconstpointer data){
	Node *a1, *a2;
	a1 = bsearch_insert(&fix->node, 'a');
	a2 = bsearch_get(&fix->node, 'a');
	g_assert(a1 != NULL);
	g_assert(a2 != NULL);
	g_assert(a1 == a2);
}

void bsearch__set2_and_get2(BSearchFixture* fix, gconstpointer data){
	Node *a1, *a2, *b1, *b2;
	Node d1, d2;

	a1 = bsearch_insert(&fix->node, 'a');
	a1->child = (void *) &d1;
	b1 = bsearch_insert(&fix->node, 'b');
	b1->child = (void *) &d2;
	a2 = bsearch_get(&fix->node, 'a');
	b2 = bsearch_get(&fix->node, 'b');

	g_assert(a2 != NULL);
	g_assert(a2->child == (void *)&d1);
	g_assert(b2 != NULL);
	g_assert(b2->child == (void *)&d2);
}

int main(int argc, char** argv) {
	g_test_init(&argc, &argv, NULL);
	g_test_add("/BSearch/set_and_get", BSearchFixture, NULL, bsearch_setup, bsearch__set_and_get, bsearch_teardown);
	g_test_add("/BSearch/set2_and_get2", BSearchFixture, NULL, bsearch_setup, bsearch__set2_and_get2, bsearch_teardown);
	//TODO:
	//delete
	//set_and_out_of_memory
	//delete_and_out_of_memory
	return g_test_run();
}

