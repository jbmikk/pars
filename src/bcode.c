
#include "bcode.h"
#include "cmemory.h"

#include <string.h>

// Rule declaration functions

#define CHUNK_SIZE 16;

void bcode_init(Bcode *bcode)
{
	bcode->frame = NULL;
	bcode->size = 0;
	bcode->assigned = 0;
}

void bcode_dispose(Bcode *bcode)
{
	c_delete(bcode->frame);
}

void bcode_add_instruction(Bcode *bcode, Instruction *instruction)
{
	int size = sizeof(Instruction);
	int available = bcode->size - bcode->assigned;
	if(available < size) {
		bcode->size += CHUNK_SIZE;
		bcode->frame = c_realloc_n(bcode->frame, bcode->size);
	}
	memcpy(bcode->frame + bcode->assigned, instruction, size);
	bcode->assigned += size;
}
