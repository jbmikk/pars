#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "input.h"
#include "token.h"


typedef struct _Lexer {
	Input *input;
	//TODO: Remove parsing state from lexer
	char mode;
	void (*handler)(struct _Lexer *lexer, Token *token);
} Lexer;

typedef void (*LexerHandler)(Lexer *lexer, Token *token);


void lexer_init(Lexer *lexer, Input *input, LexerHandler handler);
void lexer_next(Lexer *lexer, Token *token);

void identity_lexer(Lexer *lexer, Token *token);
void utf8_lexer(Lexer *lexer, Token *token);

#endif //LEXER_H
