#ifndef CRADIXTREE_H
#define	CRADIXTREE_H

#include "ctypes.h"
#include "cstruct.h"

typedef enum {
    S_DEFAULT = 0,
	S_FETCHNEXT
} CRadixTreeStatus;

cpointer c_radix_tree_get(CNode *tree, cchar *string, cuint length);
void c_radix_tree_set(CNode *tree, cchar *string, cuint length, cpointer data);
cpointer *c_radix_tree_get_next(CNode *tree, cchar *string, cuint length);
void c_radix_tree_iterator_init(CNode *tree, CIterator *iterator);
void c_radix_tree_iterator_dispose(CNode *tree, CIterator *iterator);
cpointer *c_radix_tree_iterator_next(CNode *tree, CIterator *iterator);

#endif	//CRADIXTREE_H


