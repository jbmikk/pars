#include "cradixtree.h"

#include <stddef.h>
#include <stdio.h>

#include "cmemory.h"
#include "cbsearch.h"

#define NODE_TYPE_LEAF 0
#define NODE_TYPE_TREE 1
#define NODE_TYPE_DATA 2
#define NODE_TYPE_ARRAY 3

CNode *c_radix_tree_seek(CNode *tree, cchar *string, cuint length, cuint *index, cuint *subindex)
{
    CNode *current = tree;
    cuint i = 0;
    while(i < length)
    {
        if (current->type == NODE_TYPE_TREE){
            CNode *next = c_bsearch_get(current, string[i]);
            if(next == NULL)
                break;
            current = next;
            i++;
        }
        else if (current->type == NODE_TYPE_DATA){
            current = current->child;
        }
        else if (current->type == NODE_TYPE_ARRAY){
            cchar *array = ((CDataNode*)current->child)->data;
            int array_length = current->size;
            int j;
            for (j = 0; j < array_length && i < length; j++, i++) {
                if(array[j] != string[i])
                    goto split;
            }
            if(j < array_length){
                split:
                *index = i;
                *subindex = j;
                return current;
            }
            current = current->child;
        }
        else break;
    }
    *index = i;
    return current;
}


/**
 * Add new child node after given node
 */
CNode *c_radix_tree_build_node(CNode *node, cchar *string, cuint length){
    if(length > 1)
    {
        CDataNode* array_node = c_new(CDataNode, 1);
        cchar *keys = c_malloc_n(length);
        NODE_INIT(*node, NODE_TYPE_ARRAY, length, array_node);

        //copy array
        cuint i = 0;
        for(i = 0; i < length; i++){
            keys[i] = string[i];
        }

        array_node->data = keys;
        array_node->cnode.type = NODE_TYPE_LEAF;
        node = (CNode*)array_node;
    }
    else if (length == 1)
    {
        NODE_INIT(*node, NODE_TYPE_TREE, 0, NULL);
        node = c_bsearch_insert(node, string[0]);
    }
    return node;
}

/**
 * create split array node in two using a tree node
 */
CDataNode * c_radix_tree_split(CNode *node, cchar *string, cuint length, cuint index){
    CDataNode *old = node->child, *prefix, *sufix;
    cuint old_size = node->size;
    CDataNode *data_node = c_new(CDataNode, 1);

    //make node point to new prefix array, if necessary
    node = c_radix_tree_build_node(node, old->data, index);

    if (length == 0) {
        NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
        node = &data_node->cnode;

        node = c_radix_tree_build_node(node, old->data+index, old_size-index);
        NODE_INIT(*node, old->cnode.type, old->cnode.size, old->cnode.child);
    }
    else
    {
        //make node point to new tree node
        NODE_INIT(*node, NODE_TYPE_TREE, 0, NULL);

        //add branch to hold old suffix and delete old data
        CNode *branch1 = c_bsearch_insert(node, ((cchar*)old->data)[index]);
        branch1 = c_radix_tree_build_node(branch1, old->data+index+1, old_size-(index+1));
        NODE_INIT(*branch1, old->cnode.type, old->cnode.size, old->cnode.child);

        //add branch to hold new suffix and return new node
        CNode *branch2 = c_bsearch_insert(node, string[0]);
        branch2 = c_radix_tree_build_node(branch2, string+1, length-1);

        NODE_INIT(*branch2, NODE_TYPE_DATA, 0, data_node);
        NODE_INIT(data_node->cnode, NODE_TYPE_LEAF, 0, NULL);
    }
    //delete old data
    c_free(old->data);
    c_delete(old);

    return data_node;
}

cpointer c_radix_tree_get(CNode *tree, cchar *string, cuint length)
{
    cuint i = 0, j = 0;
    CNode * node = c_radix_tree_seek(tree, string, length, &i, &j);

    if(i == length && node->type == NODE_TYPE_DATA){
        return ((CDataNode*)node->child)->data;
    }
    else return NULL;
}

void c_radix_tree_set(CNode *tree, cchar *string, cuint length, cpointer data)
{
    cuint i = 0, j = 0;
    CDataNode *data_node;

    CNode * node = c_radix_tree_seek(tree, string, length, &i, &j);

    if (node->type == NODE_TYPE_DATA) {
        data_node = (CDataNode*)node->child;
    }
    else if (node->type == NODE_TYPE_ARRAY) {
        data_node = c_radix_tree_split(node, string+i, length-i, j);
    }
    else {
        if(node->type == NODE_TYPE_TREE)
            node = c_bsearch_insert(node, string[i++]);
        node = c_radix_tree_build_node(node, string+i, length-i);
        data_node = c_new(CDataNode, 1);
        NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
        NODE_INIT(data_node->cnode, NODE_TYPE_LEAF, 0, NULL);
    }
    data_node->data = data;
}
