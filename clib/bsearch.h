#ifndef BSEARCH_H
#define	BSEARCH_H

#include "cstruct.h"

CNode *bsearch_get(CNode *parent, char key);
CNode *bsearch_insert(CNode *parent, char key);
int bsearch_delete(CNode *parent, char key);
void bsearch_delete_all(CNode *parent);

#endif	//BSEARCH_H


