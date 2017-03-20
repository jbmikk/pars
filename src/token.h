#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
	L_EOF = 0
} LToken;

typedef struct _Token {
	unsigned int index;
	unsigned int length;
	int symbol;
} Token;

void token_init(Token *token);

#endif //TOKEN_H
