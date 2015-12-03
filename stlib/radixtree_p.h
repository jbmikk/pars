#ifndef RADIXTREE_P_H
#define	RADIXTREE_P_H

#define NODE_TYPE_LEAF 0
#define NODE_TYPE_TREE 1
#define NODE_TYPE_DATA 2
#define NODE_TYPE_ARRAY 3

/**
 * Metadata for tree seeking
 * Necessary for certain operations such as removal
 */
typedef struct _ScanMetadata {
    Node *tree;
    unsigned int tree_index;
    Node *array;
    Node *parent_array;
} ScanMetadata;

#endif	//RADIXTREE_P_H


