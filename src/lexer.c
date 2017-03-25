#include "lexer.h"

void lexer_init(Lexer *lexer, Input *input, LexerFsm fsm)
{
	lexer->input = input;
	lexer->lexer_fsm = fsm;
	lexer->mode = 0;
}

void lexer_next(Lexer *lexer, Token *token)
{
	lexer->lexer_fsm(lexer, token, token);
}

void identity_lexer(Lexer *lexer, Token *t_in, Token *t_out)
{
	input_next_token(lexer->input, t_in, t_out);
}

void utf8_lexer(Lexer *lexer, Token *t_in, Token *t_out)
{
	Token t1, t2, t3;

	input_next_token(lexer->input, t_in, &t1);

	if(t1.symbol == L_EOF) {
		goto eof;
	}

	if ((t1.symbol & 0xE0) == 0xC0) {
		input_next_token(lexer->input, &t1, &t2);

		if(t2.symbol != L_EOF) {
			int symbol = ((t1.symbol & 0x1F) << 6) | (t2.symbol & 0x3F);
			token_init(t_out, t1.index, 2, symbol);
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	} else if ((t1.symbol & 0xF0) == 0xE0) {
		input_next_token(lexer->input, &t1, &t2);
		input_next_token(lexer->input, &t2, &t3);
		if(t2.symbol != L_EOF && t3.symbol != L_EOF) {
			int symbol = ((t1.symbol & 0x0F) << 12) | ((t2.symbol & 0x3F) << 6) | (t3.symbol & 0x3F);
			token_init(t_out, t1.index, 3, symbol);
		} else {
			//TODO: for now we ignore incomplete sequences
			goto eof;
		}
	}

	return;
eof:
	token_init(t_out, 0, 0, L_EOF);
	return;
}

