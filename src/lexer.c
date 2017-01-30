#include "lexer.h"

void lexer_init(Lexer *lexer, Input *input, void (*handler)(Lexer *lexer))
{
	lexer->input = input;
	lexer->token.index = 0;
	lexer->token.length = 0;
	lexer->token.symbol = 0;
	lexer->handler = handler;
}

void lexer_next(Lexer *lexer)
{
	lexer->handler(lexer);
}

void identity_lexer(Lexer *lexer)
{
	int token;
	unsigned int index;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

next_token:
	index = lexer->input->buffer_index;

	if (END(0)) {
		token = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	token = c;
	NEXT;
eof:
	lexer->token.symbol = token;
	lexer->token.index = index;
	lexer->token.length = lexer->input->buffer_index - index;
}

void utf8_lexer(Lexer *lexer)
{
	int token;
	unsigned int index;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

next_token:
	index = lexer->input->buffer_index;

	if (END(0)) {
		token = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	if ((c & 0xE0) == 0xC0) {
		unsigned char prev = c;
		NEXT;
		c = CURRENT;
		if (!END(0)) {
			token = ((prev & 0x1F) << 6) | (c & 0x3F);
			NEXT;
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	} else if ((c & 0xF0) == 0xE0) {
		unsigned char first = c;
		if (!END(2)) {
			NEXT;
			unsigned char second = CURRENT;
			NEXT;
			unsigned char third = CURRENT;
			token = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
			NEXT;
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	}
eof:
	lexer->token.symbol = token;
	lexer->token.index = index;
	lexer->token.length = lexer->input->buffer_index - index;
}

