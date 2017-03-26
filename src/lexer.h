#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "input.h"
#include "token.h"


typedef struct _Lexer {
	Input *input;
	//TODO: Remove parsing state from lexer
	char mode;
	void (*lexer_fsm)(struct _Lexer *lexer, Token *t_in, Token *t_out);
} Lexer;


void lexer_init(Lexer *lexer, Input *input);

void identity_lexer(Lexer *lexer, Token *t_in, Token *t_out);
void utf8_lexer(Lexer *lexer, Token *t_in, Token *t_out);

#endif //LEXER_H
