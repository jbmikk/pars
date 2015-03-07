#ifndef CSTRUCT
#define CSTRUCT

typedef struct _CNode {
    void *child;
    char type;
    unsigned char size;
    char key;
} CNode;

typedef struct _CDataNode {
    CNode cnode;
    void *data;
} CDataNode;

typedef struct _CIterator {
    void *key;
    unsigned char size;
    void *data;
} CIterator;

typedef struct _CScanStatus {
    void *key;
    unsigned char size;
    unsigned int index;
    unsigned int subindex;
    unsigned int type;
} CScanStatus;

typedef struct _SNode {
	void *data;
	struct _SNode *next;
} SNode;

#define NODE_INIT(V, T, S, C) ((V).type=T,(V).size=S,(V).child=C)

#endif // CSTRUCT
