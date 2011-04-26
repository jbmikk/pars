#ifndef CBSEARCH_H
#define	CBSEARCH_H

#include "ctypes.h"
#include "cstruct.h"

CNode *c_bsearch_get(CNode *parent, cchar key);
CNode *c_bsearch_insert(CNode *parent, cchar key);
void c_bsearch_delete(CNode *parent, cchar key);
void c_bsearch_delete_all(CNode *parent);

#endif	//CBSEARCH_H


