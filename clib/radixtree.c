#include "radixtree.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cmemory.h"
#include "bsearch.h"

#define NODE_TYPE_LEAF 0
#define NODE_TYPE_TREE 1
#define NODE_TYPE_DATA 2
#define NODE_TYPE_ARRAY 3

/**
 * Seek the node for the given key either for setting or getting a value
 */
CNode *radix_tree_seek(CNode *tree, CScanStatus *status)
{
	CNode *current = tree;
	cuint i = status->index;
	while(status->index < status->size)
	{
		if (current->type == NODE_TYPE_TREE){
			CNode *next = bsearch_get(current, ((cchar *)status->key)[i]);
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
CDataNode *radix_tree_scan(CNode *node, CScanStatus *status, CScanStatus *post)
{
	char *key = (char *)status->key;
	CDataNode *result = NULL;

	if (node->type == NODE_TYPE_TREE){
		CNode *children = node->child;
		CNode *current = NULL;
		cuint i = 0;

		if (status->type == S_FETCHNEXT && status->size > 0) {
			CNode *next = bsearch_get(node, key[status->index]);
			i = (next-children);
		} 

		//new index + key
		status->index++;
		post->key = c_renew(post->key, char, status->index);

		for(; i < node->size && result == NULL; i++) {
			current = children+i;
			((char *)post->key)[status->index-1] = current->key;
			result = radix_tree_scan(current, status, post);
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
				result = radix_tree_scan(node->child, status, post);
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

		result = radix_tree_scan(node->child, status, post);
		status->index -= node->size;
	} 
	return result;
}

/**
 * Add new child node after given node
 */
CNode *radix_tree_build_node(CNode *node, cchar *string, cuint length){
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
		node = bsearch_insert(node, string[0]);
	}
	return node;
}

/**
 * create split array node in two using a tree node
 */
CDataNode * radix_tree_split_array(CNode *node, CScanStatus *status)
{
	unsigned subindex = status->subindex;

	CDataNode *old = node->child, *prefix, *sufix;
	CDataNode *data_node = c_new(CDataNode, 1);

	char *old_suffix = old->data+subindex;
	char *new_suffix = status->key + status->index;
	unsigned int old_suffix_size = node->size - subindex;
	unsigned int new_suffix_size = status->size - status->index;

	//make node point to new prefix array, if necessary
	node = radix_tree_build_node(node, old->data, subindex);

	if (new_suffix_size == 0) {
		NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
		node = &data_node->cnode;

		node = radix_tree_build_node(node, old_suffix, old_suffix_size);
		NODE_INIT(*node, old->cnode.type, old->cnode.size, old->cnode.child);
	} else {
		//make node point to new tree node
		NODE_INIT(*node, NODE_TYPE_TREE, 0, NULL);

		//add branch to hold old suffix and delete old data
		CNode *branch1 = bsearch_insert(node, old_suffix[0]);
		branch1 = radix_tree_build_node(branch1, old_suffix+1, old_suffix_size-1);
		NODE_INIT(*branch1, old->cnode.type, old->cnode.size, old->cnode.child);

		//add branch to hold new suffix and return new node
		CNode *branch2 = bsearch_insert(node, new_suffix[0]);
		branch2 = radix_tree_build_node(branch2, new_suffix+1, new_suffix_size -1);

		NODE_INIT(*branch2, NODE_TYPE_DATA, 0, data_node);
		NODE_INIT(data_node->cnode, NODE_TYPE_LEAF, 0, NULL);
	}
	//delete old data
	c_free(old->data);
	c_delete(old);

	return data_node;
}

cpointer radix_tree_get(CNode *tree, cchar *string, cuint length)
{
	CScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;
	CNode * node = radix_tree_seek(tree, &status);

	if(status.index == length && node->type == NODE_TYPE_DATA){
		return ((CDataNode*)node->child)->data;
	}
	else return NULL;
}

void radix_tree_set(CNode *tree, cchar *string, cuint length, cpointer data)
{
	CDataNode *data_node;
	CScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;

	CNode * node = radix_tree_seek(tree, &status);

	if (node->type == NODE_TYPE_DATA) {
		data_node = (CDataNode*)node->child;
	} else if (node->type == NODE_TYPE_ARRAY) {
		data_node = radix_tree_split_array(node, &status);
	} else {
		if(node->type == NODE_TYPE_TREE)
			node = bsearch_insert(node, string[status.index++]);
		node = radix_tree_build_node(node, string+status.index, length-status.index);
		data_node = c_new(CDataNode, 1);
		NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
		NODE_INIT(data_node->cnode, NODE_TYPE_LEAF, 0, NULL);
	}
	data_node->data = data;
}

init_status(CScanStatus *status, CScanStatus *poststatus, cchar *string, cuint length)
{
	status->index = 0;
	status->subindex = 0;
	status->key = string;
	status->size = length;
	if(string != NULL)
		status->type = S_FETCHNEXT;
	else
		status->type = S_DEFAULT;

	poststatus->key = c_new(char, 1);
	poststatus->size = 0;
	poststatus->index = 0;
	poststatus->subindex = 0;
}

cpointer *radix_tree_get_next(CNode *tree, cchar *string, cuint length)
{
	CDataNode *res;
	CScanStatus status;
	CScanStatus poststatus;

	init_status(&status, &poststatus, string, length);

	res = radix_tree_scan(tree, &status, &poststatus);

	c_delete(poststatus.key);

	if(res != NULL) {
		return res->data;
	} else {
		return NULL;
	}
}

void radix_tree_iterator_init(CNode *tree, CIterator *iterator)
{
	iterator->key = NULL;
	iterator->size = 0;
	iterator->data= NULL;
}

void radix_tree_iterator_dispose(CNode *tree, CIterator *iterator)
{
	c_delete(iterator->key);
}

cpointer *radix_tree_iterator_next(CNode *tree, CIterator *iterator)
{
	CDataNode *res;
	CScanStatus status;
	CScanStatus poststatus;

	init_status(&status, &poststatus, iterator->key, iterator->size);

	res = radix_tree_scan(tree, &status, &poststatus);

	iterator->key = poststatus.key;
	iterator->size = poststatus.size;
	if(res != NULL) {
		return res->data;
	} else {
		return NULL;
	}
}
