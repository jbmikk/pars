#ifndef SOURCE_H
#define SOURCE_H

#include <stdio.h>
#include "token.h"

typedef struct Source {
	FILE *file;
	char *buffer;
	size_t buffer_size;
} Source;

void source_init(Source *source);
void source_dispose(Source *source);
void source_set_data(Source *source, char *data, unsigned int length);
int source_open_file(Source *source, char *pathname);
void source_next_token(Source *source, const Token *token1, Token *token2);

#endif // SOURCE_H
