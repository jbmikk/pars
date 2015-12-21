#include "radixtree.h"
#include "radixtree_p.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cmemory.h"
#include "bsearch.h"

#ifdef RADIXTREE_TRACE
#define trace(M, ...) fprintf(stderr, "RADIXTREE: " M "\n", ##__VA_ARGS__)
#define trace_node(M, NODE) \
	trace( \
		"" M "(%p) TYPE:%i, SIZE:%i, CHILD:%p", \
		NODE, NODE->type, NODE->size, NODE->child \
	);
#else
#define trace(M, ...)
#define trace_node(M, NODE)
#endif

/**
 * Seek next node in the tree matching the current scan status
 */
Node *radix_tree_seek_step(Node *tree, ScanStatus *status)
{
	Node *current = tree;

	if (current->type == NODE_TYPE_TREE) {
		trace("SEEK-TREE(%p)", current);
		//Move to the next node within the tree
		char *key = (char *)status->key;
		Node *next = bsearch_get(current, key[status->index]);
		//Break if there is no node to move to
		if(next == NULL) {
			goto NOTFOUND;
		}
		current = next;
		status->index++;
	} else if (current->type == NODE_TYPE_DATA) {
		trace("SEEK-DATA(%p)", current);
		//Just skip intermediate data nodes
		current = current->child;
	} else if (current->type == NODE_TYPE_ARRAY) {
		//Match array as far a possible
		char *key = (char *)status->key;
		char *array = ((DataNode*)current->child)->data;
		int array_length = current->size;
		int j;
		unsigned int i = status->index;
		trace("SEEK-ARRY(%p) ", current);
		for (j = 0; j < array_length && i < status->size; j++, i++) {
			//Break if a character does not match
			trace("[%c-%c]", array[j], key[i]);
			if(array[j] != key[i]) {
				status->subindex = j;
				status->index = i;
				goto NOTFOUND;
			}
		}
		//Break if it didn't match the whole array
		if(j < array_length) {
			status->subindex = j;
			status->index = i;
			goto NOTFOUND;
		}
		//The whole array was matched, move to the next node
		current = current->child;
		status->index = i;
	} else {
		trace("SEEK-LEAF(%p)", current);
		//Leaf node
		goto NOTFOUND;
	}

	status->found = status->index == status->size;
	return current;
NOTFOUND:
	trace("SEEK-NOTFOUND");
	status->found = -1;
	return current;
}

/**
 * Seek the node for the given key either for setting or getting a value
 * If the key is not found it returns the closest matching node.
 */
Node *radix_tree_seek(Node *tree, ScanStatus *status)
{
	Node *current = tree;
	unsigned int i = status->index;
	while(!status->found) {
		current = radix_tree_seek_step(current, status);
	}
	return current;
}

/**
 * Same as seek, but return tree pointers necessary for removal
 */
Node *radix_tree_seek_metadata(Node *tree, ScanStatus *status, ScanMetadata *meta)
{
	Node *current = tree;
	unsigned int i = status->index;

	meta->tree = NULL;
	meta->array = NULL;
	meta->parent_array = NULL;

	while(!status->found) {
		if (current->type == NODE_TYPE_TREE) {
			meta->tree = current;
			meta->tree_index = status->index;
			if(meta->array) {
				meta->parent_array = meta->array;
				meta->array = NULL;
			}
		} else if (current->type == NODE_TYPE_ARRAY) {
			meta->array = current;
		} else if (current->type == NODE_TYPE_DATA) {
			//If we reach a data node it's not
			//the one we are looking for
			meta->tree = NULL;
			meta->array = NULL;
			meta->parent_array = NULL;
		}

		current = radix_tree_seek_step(current, status);
	}
	return current;
}

/**
 * Scan the tree recursively for the next datanode.
 */
DataNode *radix_tree_scan(Node *node, ScanStatus *status, ScanStatus *post)
{
	char *key = (char *)status->key;
	DataNode *result = NULL;

	if (node->type == NODE_TYPE_TREE) {
		Node *children = node->child;
		Node *current = NULL;
		unsigned int i = 0;

		if (status->type == S_FETCHNEXT && status->size > 0) {
			Node *next = bsearch_get(node, key[status->index]);
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
	} else if (node->type == NODE_TYPE_DATA) {

		//if the prefix matches, then it's a match for
		if(status->type == S_DEFAULT) {
			post->size = status->index;
			result = (DataNode*)node->child;
		} else {
			unsigned int match = status->index >= status->size;
			if(match) {
				status->type = S_DEFAULT;
			}
			if(((Node*)node->child)->child != NULL) {
				result = radix_tree_scan(node->child, status, post);
			} 
		}
	} else if (node->type == NODE_TYPE_ARRAY) {

		//new index + key
		unsigned int offset = status->index;
		status->index += node->size;
		post->key = c_renew(post->key, char, status->index);

		char *array = ((DataNode*)node->child)->data;
		memcpy(post->key+offset, array, node->size);

		result = radix_tree_scan(node->child, status, post);
		status->index -= node->size;
	} 
	return result;
}

/**
 * Add new child node after given node
 */
Node *radix_tree_build_node(Node *node, char *string, unsigned int length)
{
	if(length > 1) {
		DataNode* array_node = c_new(DataNode, 1);
		char *keys = c_malloc_n(length);
		NODE_INIT(*node, NODE_TYPE_ARRAY, length, array_node);

		//copy array
		unsigned int i = 0;
		for(i = 0; i < length; i++){
			keys[i] = string[i];
		}

		array_node->data = keys;
		array_node->node.type = NODE_TYPE_LEAF;
		node = (Node*)array_node;
	} else if (length == 1) {
		NODE_INIT(*node, NODE_TYPE_TREE, 0, NULL);
		node = bsearch_insert(node, string[0]);
	} else {
		//TODO: add sentinel?
	}
	return node;
}

/**
 * create split array node in two using a tree node
 */
DataNode * radix_tree_split_node(Node *node, ScanStatus *status)
{
	unsigned subindex = status->subindex;

	DataNode *old = node->child, *prefix, *sufix;
	DataNode *data_node = c_new(DataNode, 1);

	char *old_suffix = old->data+subindex;
	char *new_suffix = status->key + status->index;
	unsigned int old_suffix_size = node->size - subindex;
	unsigned int new_suffix_size = status->size - status->index;

	//make node point to new prefix array, if necessary
	node = radix_tree_build_node(node, old->data, subindex);

	if (new_suffix_size == 0) {
                //No new suffix, then we append data node right here
		NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
		node = &data_node->node;

                //After the data node we append the old suffix
		node = radix_tree_build_node(node, old_suffix, old_suffix_size);
		NODE_INIT(*node, old->node.type, old->node.size, old->node.child);
	} else {
		//make node point to new tree node
		NODE_INIT(*node, NODE_TYPE_TREE, 0, NULL);

		//add branch to hold old suffix and delete old data
		Node *branch1 = bsearch_insert(node, old_suffix[0]);
		branch1 = radix_tree_build_node(branch1, old_suffix+1, old_suffix_size-1);
		NODE_INIT(*branch1, old->node.type, old->node.size, old->node.child);

		//add branch to hold new suffix and return new node
		Node *branch2 = bsearch_insert(node, new_suffix[0]);
		branch2 = radix_tree_build_node(branch2, new_suffix+1, new_suffix_size -1);

		NODE_INIT(*branch2, NODE_TYPE_DATA, 0, data_node);
		NODE_INIT(data_node->node, NODE_TYPE_LEAF, 0, NULL);
	}
	//delete old data
	c_free(old->data);
	c_delete(old);

	return data_node;
}

void radix_tree_compact_nodes(ScanMetadata *meta)
{
	Node *tree = meta->tree;

	//branch2 is the only branch left
	Node *branch2 = (Node*)tree->child;
	Node *parent_branch = meta->parent_array;

	Node *cont = (Node*)branch2->child;
	Node *target = NULL;

	DataNode *suffix = NULL;
	char *suffix_str = NULL;
	int suffix_size = 0;

	DataNode *prefix = NULL;
	char *prefix_str = NULL;
	int prefix_size = 0;

	if(branch2->type == NODE_TYPE_ARRAY) {
		trace("Secondary branch is array");
		//if it's an array merge it with it's parent.
		suffix_size = branch2->size;
		suffix = (DataNode*)branch2->child;
		suffix_str = suffix->data;
		target = tree;
	}

	if(parent_branch && parent_branch->type == NODE_TYPE_ARRAY) {
		trace("Parent branch is array");
		prefix_size = parent_branch->size;
		prefix = (DataNode*)parent_branch->child;
		prefix_str = prefix->data;
		target = parent_branch;
	}

	int joined_size = prefix_size + 1 + suffix_size;
	char joined[joined_size];
	int i = 0;

	if(target) {
		//Join arrays
		if(prefix) {
			for(i; i < prefix_size; i++) {
				joined[i] = prefix_str[i];
			}
		}
		joined[i++] = branch2->key;
		if(suffix) {
			int j = 0;
			for(j; j < suffix_size; j++) {
				joined[i+j] = suffix_str[j];
			}
		}

		//Build new node
		trace_node("CONT", cont);
		Node *new_branch = radix_tree_build_node(target, joined, joined_size);
		trace_node("TARG", target);
		NODE_INIT(*new_branch, cont->type, cont->size, cont->child);
		trace_node("NEW", new_branch);

		//Cleanup old nodes
		if(prefix) {
			c_free(prefix->data);
			c_delete(prefix);
		}
		c_delete(branch2);
		if(suffix) {
			c_free(suffix->data);
			c_delete(suffix);
		}
	}
}

/**
 * Remove dangling nodes and compact tree
 */
void radix_tree_clean_dangling_nodes(Node *node, ScanStatus *status, ScanMetadata *meta)
{
	trace_node("NODE", node);
	if(node->type == NODE_TYPE_LEAF) {
		//the child is a leaf

		//Delete branch1 node preceeding the leaf
		Node *branch1 = meta->array;
		if(branch1) {
			DataNode *leaf = (DataNode*)branch1->child;
			c_free(leaf->data);
			c_delete(leaf);
			branch1->type = NODE_TYPE_LEAF;
			branch1->size = 0;
		}

		//Remove childless node from tree
		Node *tree = meta->tree;
		int tree_index = meta->tree_index;
		if(tree) {
			trace_node("TREE", tree);
			char *key = (char *)status->key;
			bsearch_delete(tree, key[tree_index]);
			if(tree->size == 0) {
				//if no children there's a new leaf
				tree->type = NODE_TYPE_LEAF;
			} else if (tree->size == 1) {
				radix_tree_compact_nodes(meta);
			}

		}

	} else if(node->type == NODE_TYPE_ARRAY) {
		//We should merge it with it's parent if they are both arrays.
	}
}

void *radix_tree_get(Node *tree, char *string, unsigned int length)
{
	trace("RADIXTREE-GET(%p)", tree);
	ScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.found = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;
	Node * node = radix_tree_seek(tree, &status);

	if(status.index == length && node->type == NODE_TYPE_DATA) {
		return ((DataNode*)node->child)->data;
	} else {
		return NULL;
	}
}

void radix_tree_set(Node *tree, char *string, unsigned int length, void *data)
{
	trace("RADIXTREE-SET(%p)", tree);
	DataNode *data_node;
	ScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.found = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;

	Node * node = radix_tree_seek(tree, &status);

	if (node->type == NODE_TYPE_DATA) {
		data_node = (DataNode*)node->child;
	} else if (node->type == NODE_TYPE_ARRAY) {
		data_node = radix_tree_split_node(node, &status);
	} else {
		if(node->type == NODE_TYPE_TREE)
			node = bsearch_insert(node, string[status.index++]);
		node = radix_tree_build_node(node, string+status.index, length-status.index);
		data_node = c_new(DataNode, 1);
		NODE_INIT(*node, NODE_TYPE_DATA, 0, data_node);
		NODE_INIT(data_node->node, NODE_TYPE_LEAF, 0, NULL);
	}
	data_node->data = data;
}

void radix_tree_remove(Node *tree, char *string, unsigned int length)
{
	trace("RADIXTREE-REMOVE(%p)", tree);
	DataNode *data_node;
	ScanStatus status;
	status.index = 0;
	status.subindex = 0;
	status.found = 0;
	status.key = string;
	status.size = length;
	status.type = S_DEFAULT;

	ScanMetadata meta;

	Node * node = radix_tree_seek_metadata(tree, &status, &meta);

	trace_node("ROOT", tree);
	trace_node("NODE", node);
	if(status.index == length && node->type == NODE_TYPE_DATA) {
		//Delete data node, parent node points to datanode's children.
		data_node = (DataNode*)node->child;
		node->type = data_node->node.type;
		node->child = data_node->node.child;
		c_free(data_node);
		
		radix_tree_clean_dangling_nodes(node, &status, &meta);
	}
}

init_status(ScanStatus *status, ScanStatus *poststatus, char *string, unsigned int length)
{
	status->index = 0;
	status->subindex = 0;
	status->found = 0;
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
	poststatus->found = 0;
}

void **radix_tree_get_next(Node *tree, char *string, unsigned int length)
{
	DataNode *res;
	ScanStatus status;
	ScanStatus poststatus;

	init_status(&status, &poststatus, string, length);

	res = radix_tree_scan(tree, &status, &poststatus);

	c_delete(poststatus.key);

	if(res != NULL) {
		return res->data;
	} else {
		return NULL;
	}
}

void radix_tree_iterator_init(Node *tree, Iterator *iterator)
{
	iterator->key = NULL;
	iterator->size = 0;
	iterator->data= NULL;
}

void radix_tree_iterator_dispose(Node *tree, Iterator *iterator)
{
	c_delete(iterator->key);
}

void **radix_tree_iterator_next(Node *tree, Iterator *iterator)
{
	DataNode *res;
	ScanStatus status;
	ScanStatus poststatus;

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
