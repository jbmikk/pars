#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "input.h"

typedef enum {
	L_EOF = 0,
	L_DEFINING_SYMBOL = '=',
	L_CONCATENATE_SYMBOL = ',',
	L_TERMINATOR_SYMBOL = ';',
	L_DEFINITION_SEPARATOR_SYMBOL = '|',
	L_START_GROUP_SYMBOL = '(',
	L_END_GROUP_SYMBOL = ')',
	L_WHITE_SPACE = 256,
	L_INTEGER,
	L_IDENTIFIER,
	L_TERMINAL_STRING
} LToken;

typedef struct _Lexer {
	unsigned int index;
	unsigned int length;
	int symbol;
	Input *input;
} Lexer;

void lexer_init(Lexer *lexer, Input *input);
void lexer_next(Lexer *lexer);

#endif //LEXER_H
