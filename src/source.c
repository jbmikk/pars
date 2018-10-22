#include "source.h"
#include "cmemory.h"
#include "dbg.h"

#include <stdlib.h>

void source_init(Source *source)
{
	source->file = NULL;
	source->buffer = NULL;
	source->buffer_size = 0;
}

void source_set_data(Source *source, char *data, unsigned int length)
{
	source->buffer = data;
	source->buffer_size = length;
}

int source_open_file(Source *source, char *pathname)
{
	FILE *file;
	char *buffer;
	unsigned int length;

	file = fopen(pathname, "rb");
	check(file, "Could not open file: %s", pathname);

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	if(length > 0) {
		buffer = malloc(sizeof(char) * length);
		length = fread(buffer, 1, length, file);
	} else {
		buffer = NULL;
	}

	source_set_data(source, buffer, length);
	source->file = file;

	return 0;
error:
	return -1;
}

void source_dispose(Source *source)
{
	if(source->file) {
		free(source->buffer);
		source->buffer = NULL;
		fclose(source->file);
		source->file = NULL;
	}
}

void source_next_token(Source *source, const Token *token1, Token *token2)
{
	unsigned int index = token1->index + token1->length;
	token2->index = index;
	if(index < source->buffer_size) {
		token2->length = 1;
		token2->symbol = source->buffer[index];
	} else {
		token2->length = 0;
		token2->symbol = L_EOF;
	}
}

