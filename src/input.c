#include "input.h"
#include "cmemory.h"

void input_init_buffer(Input *input, char *data, unsigned int length)
{
	input->file = NULL;
	input->buffer_size = length;
	input->buffer = data;
}

void input_init(Input *input, char *pathname)
{
	FILE *file;
	char *buffer;
	unsigned int length;

	file = fopen(pathname, "rb");

	if(file)
	{
		fseek(file, 0, SEEK_END);
		length = ftell(file);
		fseek(file, 0, SEEK_SET);
		if(length > 0) {
			buffer = c_new(char, length);
			length = fread(buffer, 1, length, file);
		} else {
			buffer = NULL;
		}
		//TODO:close file
		input_init_buffer(input, buffer, length);
		input->file = file;
		input->is_open = 1;
	} else {
		input_init_buffer(input, NULL, 0);
		input->file = NULL;
		input->is_open = 0;
	}
}

void input_dispose(Input *input)
{
	if(input->is_open) {
		c_delete(input->buffer);
		input->buffer = NULL;
		fclose(input->file);
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

