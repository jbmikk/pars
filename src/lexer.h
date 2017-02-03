#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "input.h"

typedef enum {
	L_EOF = 0
} LToken;

typedef struct _Token {
	unsigned int index;
	unsigned int length;
	int symbol;
} Token;

typedef struct _Lexer {
	Input *input;
	void (*handler)(struct _Lexer *lexer, Token *token);
} Lexer;

typedef void (*LexerHandler)(Lexer *lexer, Token *token);


void token_init(Token *token);

void lexer_init(Lexer *lexer, Input *input, LexerHandler handler);
void lexer_next(Lexer *lexer, Token *token);

void identity_lexer(Lexer *lexer, Token *token);
void utf8_lexer(Lexer *lexer, Token *token);

#endif //LEXER_H
