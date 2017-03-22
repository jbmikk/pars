#include "lexer.h"

void lexer_init(Lexer *lexer, Input *input, LexerFsm fsm)
{
	lexer->input = input;
	lexer->lexer_fsm = fsm;
	lexer->mode = 0;
}

void lexer_next(Lexer *lexer, Token *token)
{
	lexer->lexer_fsm(lexer, token);
}

void identity_lexer(Lexer *lexer, Token *token)
{
	int symbol;
	unsigned int index;
	unsigned char c;

	index = lexer->input->buffer_index;

	if (input_end(lexer->input, 0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = input_get_current(lexer->input);
	symbol = c;
	input_next(lexer->input);
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

	index = lexer->input->buffer_index;

	if (input_end(lexer->input, 0)) {
		symbol = L_EOF;
		lexer->input->eof = 1;
		goto eof;
	}

	c = input_get_current(lexer->input);
	if ((c & 0xE0) == 0xC0) {
		unsigned char prev = c;
		input_next(lexer->input);
		c = input_get_current(lexer->input);
		if (!input_end(lexer->input, 0)) {
			symbol = ((prev & 0x1F) << 6) | (c & 0x3F);
			input_next(lexer->input);
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	} else if ((c & 0xF0) == 0xE0) {
		unsigned char first = c;
		if (!input_end(lexer->input, 2)) {
			input_next(lexer->input);
			unsigned char second = input_get_current(lexer->input);
			input_next(lexer->input);
			unsigned char third = input_get_current(lexer->input);
			symbol = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
			input_next(lexer->input);
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

