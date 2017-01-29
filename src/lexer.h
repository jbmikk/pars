#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "input.h"

typedef enum {
	L_EOF = 0
} LToken;

typedef struct _Lexer {
	unsigned int index;
	unsigned int length;
	int symbol;
	Input *input;
	void (*handler)(struct _Lexer *lexer);
} Lexer;

void lexer_init(Lexer *lexer, Input *input, void (*handler)(Lexer *lexer));
void lexer_next(Lexer *lexer);

void identity_lexer(Lexer *lexer);
void utf8_lexer(Lexer *lexer);

#endif //LEXER_H
