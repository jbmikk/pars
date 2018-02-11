#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
	L_EOF = 0
} LToken;

typedef struct Token {
	unsigned int index;
	unsigned int length;
	int symbol;
} Token;

void token_init(Token *token, unsigned int index, unsigned int length, int symbol);

#endif //TOKEN_H
