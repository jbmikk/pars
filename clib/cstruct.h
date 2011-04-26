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
#endif // CSTRUCT
