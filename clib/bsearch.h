#ifndef BSEARCH_H
#define	BSEARCH_H

#include "ctypes.h"
#include "cstruct.h"

CNode *bsearch_get(CNode *parent, cchar key);
CNode *bsearch_insert(CNode *parent, cchar key);
int bsearch_delete(CNode *parent, cchar key);
void bsearch_delete_all(CNode *parent);

#endif	//BSEARCH_H


