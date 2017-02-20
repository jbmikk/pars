#include "lexer.h"

void token_init(Token *token)
{
	token->symbol = 0;
	token->index = 0;
	token->length = 0;
}

void lexer_init(Lexer *lexer, Input *input, LexerHandler handler)
{
	lexer->input = input;
	lexer->handler = handler;
}

void lexer_next(Lexer *lexer, Token *token)
{
	lexer->handler(lexer, token);
}

void identity_lexer(Lexer *lexer, Token *token)
{
	int symbol;
	unsigned int index;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

	index = lexer->input->buffer_index;

	if (END(0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	symbol = c;
	NEXT;
eof:
	token->symbol = symbol;
	token->index = index;
	token->length = lexer->input->buffer_index - index;
}

void utf8_lexer(Lexer *lexer, Token *token)
{
	int symbol;
	unsigned int index;
	unsigned char c;

#define CURRENT (lexer->input->buffer[lexer->input->buffer_index])
#define NEXT (++lexer->input->buffer_index)
#define END(V) (lexer->input->buffer_index >= lexer->input->buffer_size-V)

	index = lexer->input->buffer_index;

	if (END(0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = CURRENT;
	if ((c & 0xE0) == 0xC0) {
		unsigned char prev = c;
		NEXT;
		c = CURRENT;
		if (!END(0)) {
			symbol = ((prev & 0x1F) << 6) | (c & 0x3F);
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
			symbol = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
			NEXT;
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	}
eof:
	token->symbol = symbol;
	token->index = index;
	token->length = lexer->input->buffer_index - index;
}

