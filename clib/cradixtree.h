#ifndef CRADIXTREE_H
#define	CRADIXTREE_H

#include "ctypes.h"
#include "cstruct.h"

cpointer c_radix_tree_get(CNode *tree, cchar *string, cuint length);
void c_radix_tree_set(CNode *tree, cchar *string, cuint length, cpointer data);

#endif	//CRADIXTREE_H


