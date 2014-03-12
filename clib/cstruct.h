#ifndef CSTRUCT
#define CSTRUCT

#include "ctypes.h"

typedef struct _CNode {
    cpointer child;
    cchar type;
    cuchar size;
    cchar key;
} CNode;

typedef struct _CDataNode {
    CNode cnode;
    cpointer data;
} CDataNode;

typedef struct _CIterator {
    cpointer key;
    cuchar size;
    cpointer data;
} CIterator;

typedef struct _CScanStatus {
    cpointer key;
    cuchar size;
    cuint index;
    cuint subindex;
    cuint type;
} CScanStatus;

typedef struct _SNode {
	cpointer data;
	struct _SNode *next;
} SNode;

#define NODE_INIT(V, T, S, C) ((V).type=T,(V).size=S,(V).child=C)

#endif // CSTRUCT
