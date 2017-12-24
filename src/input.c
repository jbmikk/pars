#include "input.h"
#include "cmemory.h"
#include "dbg.h"

#include <stdlib.h>

void input_init(Input *input)
{
	input->file = NULL;
	input->buffer = NULL;
	input->buffer_size = 0;
}

void input_set_data(Input *input, char *data, unsigned int length)
{
	input->buffer = data;
	input->buffer_size = length;
}

int input_open_file(Input *input, char *pathname)
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

	input_set_data(input, buffer, length);
	input->file = file;

	return 0;
error:
	return -1;
}

void input_dispose(Input *input)
{
	if(input->file) {
		free(input->buffer);
		input->buffer = NULL;
		fclose(input->file);
		input->file = NULL;
	}
}

void input_next_token(Input *input, const Token *token1, Token *token2)
{
	unsigned int index = token1->index + token1->length;
	token2->index = index;
	if(index < input->buffer_size) {
		token2->length = 1;
		token2->symbol = input->buffer[index];
	} else {
		token2->length = 0;
		token2->symbol = L_EOF;
	}
}

