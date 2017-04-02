#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include "token.h"

typedef struct _Input {
	FILE *file;
	unsigned char is_open;
	char *buffer;
	size_t buffer_size;
} Input;

void input_init_buffer(Input *input, char *data, unsigned int length);
void input_init(Input *input, char *pathname);
void input_dispose(Input *input);
void input_next_token(Input *input, const Token *token1, Token *token2);

#endif // INPUT_H
