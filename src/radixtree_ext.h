#ifndef RADIXTREE_EXT_H
#define RADIXTREE_EXT_H

#include "radixtree.h"

void *radix_tree_get_int(Node *tree, int number);
void radix_tree_set_int(Node *tree, int number, void *data);
void *radix_tree_get_next_int(Node *tree, int number);

#endif //RADIXTREE_EXT_H
