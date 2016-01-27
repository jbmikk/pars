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


void radix_tree_init(Node *tree, char type, unsigned char size, void *child)
{
	tree->type = type;
	tree->size = size;
	tree->child = child;
}

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

void scan_metadata_init(ScanMetadata *meta) 
{
	meta->previous3 = NULL;
	meta->previous2 = NULL;
	meta->previous = NULL;
}

void scan_metadata_push(ScanMetadata *meta, Node *node, int index) 
{
	meta->previous3 = meta->previous2;
	meta->previous2 = meta->previous;
	meta->previous = node;
	meta->p_index3 = meta->p_index2;
	meta->p_index2 = meta->p_index;
	meta->p_index = index;
}

void scan_metadata_pop(ScanMetadata *meta) 
{
	meta->previous = meta->previous2;
	meta->previous2 = meta->previous3;
	meta->previous3 = NULL;
	meta->p_index = meta->p_index2;
	meta->p_index2 = meta->p_index3;
	meta->p_index3 = 0;
}

/**
 * Same as seek, but return tree pointers necessary for removal
 */
Node *radix_tree_seek_metadata(Node *tree, ScanStatus *status, ScanMetadata *meta)
{
	Node *current = tree;
	unsigned int i = status->index;

	scan_metadata_init(meta);

	while(!status->found) {
		if (current->type == NODE_TYPE_DATA) {
			scan_metadata_init(meta);
		} else {
			scan_metadata_push(meta, current, status->index);
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
		radix_tree_init(node, NODE_TYPE_ARRAY, length, array_node);

		//copy array
		unsigned int i = 0;
		for(i = 0; i < length; i++){
			keys[i] = string[i];
		}

		array_node->data = keys;
		array_node->node.type = NODE_TYPE_LEAF;
		node = (Node*)array_node;
	} else if (length == 1) {
		radix_tree_init(node, NODE_TYPE_TREE, 0, NULL);
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
		radix_tree_init(node, NODE_TYPE_DATA, 0, data_node);
		node = &data_node->node;

                //After the data node we append the old suffix
		node = radix_tree_build_node(node, old_suffix, old_suffix_size);
		radix_tree_init(node, old->node.type, old->node.size, old->node.child);
	} else {
		//make node point to new tree node
		radix_tree_init(node, NODE_TYPE_TREE, 0, NULL);

		//add branch to hold old suffix and delete old data
		Node *branch1 = bsearch_insert(node, old_suffix[0]);
		branch1 = radix_tree_build_node(branch1, old_suffix+1, old_suffix_size-1);
		radix_tree_init(branch1, old->node.type, old->node.size, old->node.child);

		//add branch to hold new suffix and return new node
		Node *branch2 = bsearch_insert(node, new_suffix[0]);
		branch2 = radix_tree_build_node(branch2, new_suffix+1, new_suffix_size -1);

		radix_tree_init(branch2, NODE_TYPE_DATA, 0, data_node);
		radix_tree_init(&data_node->node, NODE_TYPE_LEAF, 0, NULL);
	}
	//delete old data
	c_free(old->data);
	c_delete(old);

	return data_node;
}

void radix_tree_compact_nodes(Node *node1, Node *node2, Node *node3)
{
	Node *target = NULL;
	Node *cont = (Node*)node3->child;
	int nodes = 0;
	int joined_size = 0;

	char *node1_str = NULL;
	int node1_size = 0;
	int node1_type = 0;
	Node *node1_old;

	int node2_size = 0;
	Node *node2_old;

	char *node3_str = NULL;
	int node3_size = 0;
	int node3_type = 0;
	Node *node3_old;

	if(node3) {
		if(node3->type == NODE_TYPE_ARRAY) {
			trace("Node 3 is array");
			node3_str = ((DataNode*)node3->child)->data;
			node3_size = node3->size;
			joined_size += node3_size;
			nodes++;
		} else if(node3->type == NODE_TYPE_TREE && node3->size == 1) {
			trace("Node 3 is tree");
			node3_str = &((Node *)node3->child)->key;
			node3_size = 1;
			joined_size += node3_size;
			nodes++;
		}
		node3_type = node3->type;
		node3_old = node3->child;
	}

	if(node2 && node2->type == NODE_TYPE_TREE) {
		trace("Node 2 is tree");
		node2_size = 1;
		joined_size += node2_size;
		nodes++;
		target = node2;
		node2_old = node2->child;
	}

	if(node1) {
		if(node1->type == NODE_TYPE_ARRAY) {
			trace("Node 1 is array");
			node1_str = ((DataNode*)node1->child)->data;
			node1_size = node1->size;
			joined_size += node1_size;
			nodes++;
			target = node1;
		} else if(node1->type == NODE_TYPE_TREE && node1->size == 1) {
			trace("Node 1 is tree");
			node1_str = &((Node *)node1->child)->key;
			node1_size = 1;
			joined_size += node1_size;
			nodes++;
			target = node1;
		}
		node1_type = node3->type;
		node1_old = node1->child;
	}

	char joined[joined_size];
	int i = 0;

	if(nodes > 1) {
		//Join arrays
		for(i; i < node1_size; i++) {
			joined[i] = node1_str[i];
		}
		if(node2_size) {
			joined[i++] = ((Node *)node2->child)->key;
		}
		int j = 0;
		for(j; j < node3_size; j++) {
			joined[i+j] = node3_str[j];
		}

		//Build new node
		trace("new size: %i", joined_size);
		Node *new_branch = radix_tree_build_node(target, joined, joined_size);
		radix_tree_init(new_branch, cont->type, cont->size, cont->child);
		trace_node("CONT", cont);
		trace_node("NEW", new_branch);
		trace_node("TARG", target);

		//Cleanup old nodes
		if(node1_size) {
			if(node1_type == NODE_TYPE_ARRAY) {
				c_free(node1_str);
			}
			c_delete(node1_old);
		}
		if(node2_size) {
			c_delete(node2_old);
		}
		if(node3_size) {
			if(node3_type == NODE_TYPE_ARRAY) {
				c_free(node3_str);
			}
			c_delete(node3_old);
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

		//Delete node preceeding the leaf
		Node *previous = meta->previous;
		if(previous && previous->type == NODE_TYPE_ARRAY) {
			DataNode *leaf = (DataNode*)previous->child;
			c_free(leaf->data);
			c_delete(leaf);
			previous->type = NODE_TYPE_LEAF;
			previous->size = 0;
			scan_metadata_pop(meta);
		} else if(previous && previous->type == NODE_TYPE_TREE && previous->size == 1) {
			c_delete(previous->child);
			previous->type = NODE_TYPE_LEAF;
			previous->size = 0;
			scan_metadata_pop(meta);
		}

		//Remove childless node from pivot
		if(meta->previous && meta->previous->type == NODE_TYPE_TREE) {
			Node *pivot = meta->previous;
			int pivot_index = meta->p_index;

			trace_node("PIVOT", pivot);
			char *key = (char *)status->key;
			bsearch_delete(pivot, key[pivot_index]);
			if (pivot->size == 1) {
				radix_tree_compact_nodes(meta->previous2, pivot, pivot->child);
			}

		}

	} else {
		radix_tree_compact_nodes(meta->previous, NULL, node);
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
		trace("FOUND %p, %p", node, node->child);
		return ((DataNode*)node->child)->data;
	} else {
		trace("NOTFOUND");
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
		radix_tree_init(node, NODE_TYPE_DATA, 0, data_node);
		radix_tree_init(&data_node->node, NODE_TYPE_LEAF, 0, NULL);
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
		node->size = data_node->node.size;
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

void radix_tree_dispose(Node *tree)
{
	if(tree->type == NODE_TYPE_TREE) {
		Node *child = (Node *)tree->child;
		int i;
		for(i = 0; i < tree->size; i++) {
			radix_tree_dispose(child+i);
		}
		c_delete(tree->child);
	} else if(tree->type == NODE_TYPE_ARRAY) {
		radix_tree_dispose((Node *)tree->child);
		c_free(((DataNode*)tree->child)->data);
		c_delete(tree->child);
	} else if(tree->type == NODE_TYPE_DATA) {
		radix_tree_dispose((Node *)tree->child);
		c_delete(tree->child);
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

	if(iterator->key) {
		c_delete(iterator->key);
	}

	iterator->key = poststatus.key;
	iterator->size = poststatus.size;

	if(res != NULL) {
		return res->data;
	} else {
		return NULL;
	}
}
