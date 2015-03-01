#include "bsearch.h"

#include <stddef.h>
#include <stdio.h>

#include "cmemory.h"

CNode *bsearch_get(CNode *parent, cchar key)
{
	CNode *children = parent->child;
	CNode *next;
	cchar left = 0;
	cchar right = parent->size-1;

	while(left <= right) {
		cchar i = left+((right - left)>>1);
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

CNode *bsearch_insert(CNode *parent, cchar key)
{
	CNode *new_children = c_new(CNode, parent->size+1);
	CNode *new_node;
	CNode *src = parent->child;
	CNode *dst = new_children;
	CNode *end = src+parent->size;

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
}

void bsearch_delete(CNode *parent, cchar key)
{
	if(bsearch_get(parent, key)!= NULL) {
		CNode *new_children = NULL;
		if(parent->size > 1) {
			new_children = c_new(CNode, parent->size-1);
			CNode *dst = new_children;
			CNode *src = parent->child;
			CNode *end = src+parent->size;
			while(src < end && src->key < key)
				*dst++ = *src++;
			src++;
			while(src < end)
				*dst++ = *src++;
		}
		c_free(parent->child);

		//assign new children
		parent->child = new_children;
		parent->size--;
	}
}

void bsearch_delete_all(CNode *parent)
{
	if(parent->child != NULL) {
		c_free(parent->child);
		parent->child = NULL;
		parent->size = 0;
	}
}
