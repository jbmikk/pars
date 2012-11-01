#include "cradixtree.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cmemory.h"
#include "cbsearch.h"

#define NODE_TYPE_LEAF 0
#define NODE_TYPE_TREE 1
#define NODE_TYPE_DATA 2
#define NODE_TYPE_ARRAY 3

/**
 * Seek the node for the given key either for setting or getting a value
 */
CNode *c_radix_tree_seek(CNode *tree, CScanStatus *status)
{
    CNode *current = tree;
	cuint i = status->index;
    while(status->index < status->size)
    {
		if (current->type == NODE_TYPE_TREE){
			CNode *next = c_bsearch_get(current, ((cchar *)status->key)[i]);
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
			for (j = 0; j < array_length && i < status->size; j++, i++) {
				if(array[j] != ((cchar *)status->key)[i]) {
					status->subindex = j;
					break;
				}
			}
			if(j < array_length){
				status->subindex = j;
				break;
			}
			current = current->child;
		} else {
			break;
		}
		status->index = i;
    }
	status->index = i;
    return current;
}

/**
 * Scan the tree recursively for the next datanode.
 */
CDataNode *c_radix_tree_scan(CNode *node, CScanStatus *status, CScanStatus *post)
{
	char *key = (char *)status->key;
	CDataNode *result = NULL;

	if (node->type == NODE_TYPE_TREE){
		CNode *children = node->child;
		CNode *current = NULL;
		cuint i = 0;

		if (status->type == S_FETCHNEXT && status->size > 0) {
			CNode *next = c_bsearch_get(node, key[status->index]);
			i = (next-children);
		} 

		//new index + key
		status->index++;
		post->key = c_renew(post->key, char, status->index);

		for(; i < node->size && result == NULL; i++) {
			current = children+i;
			((char *)post->key)[status->index-1] = current->key;
			result = c_radix_tree_scan(current, status, post);
			if(result != NULL)
				break;
		}
		status->index--;
	}
	else if (node->type == NODE_TYPE_DATA){
		//if the prefix matches, then it's a match for
		if(status->type == S_DEFAULT) {
			post->size = status->index;
			result = (CDataNode*)node->child;
		} else {
			cuint match = status->index >= status->size;
			if(match) {
				status->type = S_DEFAULT;
			}
			if(((CNode*)node->child)->child != NULL) {
				result = c_radix_tree_scan(node->child, status, post);
			} 
		}
	}
	else if (node->type == NODE_TYPE_ARRAY){
		//new index + key
		cuint offset = status->index;
		status->index += node->size;
		post->key = c_renew(post->key, char, status->index);

		cchar *array = ((CDataNode*)node->child)->data;
		memcpy(post->key+offset, array, node->size);

		result = c_radix_tree_scan(node->child, status, post);
		status->index -= node->size;
	} 
	return result;
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
	CScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;
    CNode * node = c_radix_tree_seek(tree, &status);

    if(status.index == length && node->type == NODE_TYPE_DATA){
        return ((CDataNode*)node->child)->data;
    }
    else return NULL;
}

void c_radix_tree_set(CNode *tree, cchar *string, cuint length, cpointer data)
{
    CDataNode *data_node;
	CScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;

    CNode * node = c_radix_tree_seek(tree, &status);

    if (node->type == NODE_TYPE_DATA) {
        data_node = (CDataNode*)node->child;
    }
    else if (node->type == NODE_TYPE_ARRAY) {
        data_node = c_radix_tree_split(node, string+status.index, length-status.index, status.subindex);
    }
    else {
        if(node->type == NODE_TYPE_TREE)
            node = c_bsearch_insert(node, string[status.index++]);
        node = c_radix_tree_build_node(node, string+status.index, length-status.index);
        data_node = c_new(CDataNode, 1);
        NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
        NODE_INIT(data_node->cnode, NODE_TYPE_LEAF, 0, NULL);
    }
    data_node->data = data;
}

void c_radix_tree_iterator_init(CNode *tree, CIterator *iterator)
{
	iterator->key = NULL;
	iterator->size = 0;
	iterator->data= NULL;
}

void c_radix_tree_iterator_dispose(CNode *tree, CIterator *iterator)
{
	c_delete(iterator->key);
}

cpointer *c_radix_tree_iterator_next(CNode *tree, CIterator *iterator)
{
	CDataNode *res;
	CScanStatus status;
	CScanStatus poststatus;
	status.index = 0;
	status.subindex = 0;

	if(iterator->key != NULL) {
		status.key = iterator->key;
		status.size = iterator->size;
		status.type = S_FETCHNEXT;
	} else {
		status.key = NULL;
		status.size = 0;
		status.type = S_DEFAULT;
	}
	poststatus.key = c_new(char, 1);
	poststatus.size = 0;
	poststatus.index = 0;
	poststatus.subindex = 0;

	res = c_radix_tree_scan(tree, &status, &poststatus);

	iterator->key = poststatus.key;
	iterator->size = poststatus.size;
	if(res != NULL) {
		return res->data;
	} else {
		return NULL;
	}
}
