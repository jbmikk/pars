#ifndef RADIXTREE_H
#define	RADIXTREE_H

#include "structs.h"

typedef enum {
	S_DEFAULT = 0,
	S_FETCHNEXT
} RadixTreeStatus;

void radix_tree_init(Node *tree, char type, unsigned char size, void *child);
void *radix_tree_get(Node *tree, char *string, unsigned int length);
void radix_tree_set(Node *tree, char *string, unsigned int length, void *data);
void **radix_tree_get_next(Node *tree, char *string, unsigned int length);
void radix_tree_remove(Node *tree, char *string, unsigned int length);
void radix_tree_dispose(Node *tree);
void radix_tree_iterator_init(Node *tree, Iterator *iterator);
void radix_tree_iterator_dispose(Node *tree, Iterator *iterator);
void **radix_tree_iterator_next(Node *tree, Iterator *iterator);

#endif	//RADIXTREE_H


