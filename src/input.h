#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>
#include "token.h"

typedef struct _Input {
	FILE *file;
	char *buffer;
	size_t buffer_size;
} Input;

void input_init(Input *input);
void input_dispose(Input *input);
void input_set_data(Input *input, char *data, unsigned int length);
int input_open_file(Input *input, char *pathname);
void input_next_token(Input *input, const Token *token1, Token *token2);

#endif // INPUT_H
