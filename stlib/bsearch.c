#include "bsearch.h"

#include <stddef.h>
#include <stdio.h>

#include "cmemory.h"
#include "dbg.h"

Node *bsearch_get(Node *parent, char key)
{
	Node *children = parent->child;
	Node *next;
	char left = 0;
	char right = parent->size-1;

	while(left <= right) {
		char i = left+((right - left)>>1);
		if(children[i].key < key) {
			left = i+1;
		} else if(children[i].key > key) {
			right = i-1;
		} else {
			next = &(children[i]);
			return next;
		}
	}
	return NULL;
}

Node *bsearch_insert(Node *parent, char key)
{
	Node *new_children;
	Node *new_node;
	Node *src = parent->child;
	Node *dst;
	Node *end = src+parent->size;

	dst = new_children = c_new(Node, parent->size+1);
	check_mem(new_children);

	if(src != NULL) {
		while(src < end && src->key < key)
			*dst++ = *src++;
		new_node = dst++;
		while(src < end)
			*dst++ = *src++;

		c_free(parent->child);
	} else {
		new_node = new_children;
	}

	new_node->key = key;

	//assign new children
	parent->child = new_children;
	parent->size++;

	return new_node;
error:
	return NULL;
}

int bsearch_delete(Node *parent, char key)
{
	if(bsearch_get(parent, key)!= NULL) {
		Node *new_children = NULL;
		if(parent->size > 1) {
			new_children = c_new(Node, parent->size-1);
			check_mem(new_children);

			Node *dst = new_children;
			Node *src = parent->child;
			Node *end = src+parent->size;
			while(src < end && src->key < key)
				*dst++ = *src++;
			src++;
			while(src < end)
				*dst++ = *src++;
		}

		//replace parent's children
		c_free(parent->child);
		parent->child = new_children;
		parent->size--;
	}
	return 0;
error:
	return -1;
}

void bsearch_delete_all(Node *parent)
{
	if(parent->child != NULL) {
		c_free(parent->child);
		parent->child = NULL;
		parent->size = 0;
	}
}
