#include "radixtree_ext.h"
#include "arrays.h"

void *radix_tree_get_int(Node *tree, int number)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	int_to_array(buffer, &size, number);
	return radix_tree_get(tree, buffer, size);
}

void radix_tree_set_int(Node *tree, int number, void *data)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	int_to_array(buffer, &size, number);
	radix_tree_set(tree, buffer, size, data);
}

void *radix_tree_get_next_int(Node *tree, int number)
{
	unsigned char buffer[sizeof(int)];
	unsigned int size;
	int_to_array(buffer, &size, number);
	return radix_tree_get_next(tree, buffer, size);
}
