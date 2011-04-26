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

#define NODE_INIT(V, T, S, C) ((V).type=T,(V).size=S,(V).child=C)

#endif // CSTRUCT
