#ifndef RADIXTREE_H
#define	RADIXTREE_H

#include "cstruct.h"

typedef enum {
	S_DEFAULT = 0,
	S_FETCHNEXT
} CRadixTreeStatus;

void *radix_tree_get(CNode *tree, char *string, unsigned int length);
void radix_tree_set(CNode *tree, char *string, unsigned int length, void *data);
void **radix_tree_get_next(CNode *tree, char *string, unsigned int length);
void radix_tree_iterator_init(CNode *tree, CIterator *iterator);
void radix_tree_iterator_dispose(CNode *tree, CIterator *iterator);
void **radix_tree_iterator_next(CNode *tree, CIterator *iterator);

#endif	//RADIXTREE_H


