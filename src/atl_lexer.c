#include "atl_lexer.h"

void atl_lexer(Lexer *lexer, Token *t_in, Token *t_out)
{
#define BETWEEN(V, A, B) (V >= A && V <= B)
#define IS_SPACE(V) (V == ' ' || V == '\t' || V == '\n' || V == '\r' || V =='\f')

	Token t1, t2, tn;

next_token:
	input_next_token(lexer->input, t_in, &t1);

	if(t1.symbol == L_EOF) {
		goto eof;
	}

	if (IS_SPACE(t1.symbol)) {
		tn = t1;
		while(1) {
			input_next_token(lexer->input, &tn, &tn);
			if(!IS_SPACE(tn.symbol)) {
				break;
			}
		}
		token_init(t_out, t1.index, tn.index - t1.index, ATL_WHITE_SPACE);
	} else if (BETWEEN(t1.symbol, 'a', 'z') || BETWEEN(t1.symbol, 'A', 'Z')) {
		tn = t1;
		while(1) {
			input_next_token(lexer->input, &tn, &tn);
			if(
				!BETWEEN(tn.symbol, 'a', 'z') &&
				!BETWEEN(tn.symbol, 'A', 'Z') &&
				!BETWEEN(tn.symbol, '0', '9')
			) {
				break;
			}
		}
		token_init(t_out, t1.index, tn.index - t1.index, ATL_IDENTIFIER);
	} else if (BETWEEN(t1.symbol, '0', '9')) {
		tn = t1;
		while(1) {
			input_next_token(lexer->input, &tn, &tn);
			if(!BETWEEN(tn.symbol, '0', '9'))
				break;
		}
		token_init(t_out, t1.index, tn.index - t1.index, ATL_INTEGER);
	} else if (t1.symbol == '"') {
		tn = t1;
		while(1) {
			int prev = tn.symbol;
			input_next_token(lexer->input, &tn, &tn);
			if(tn.symbol == '"' && prev != '\\') {
				break;
			}
		}
		token_init(t_out, t1.index, tn.index - t1.index + 1, ATL_STRING);
	} else if (t1.symbol == '\'') {
		tn = t1;
		while(1) {
			int prev = tn.symbol;
			input_next_token(lexer->input, &tn, &tn);
			if(tn.symbol == '\'' && prev != '\\') {
				break;
			}
		}
		token_init(t_out, t1.index, tn.index - t1.index + 1, ATL_STRING);
	} else if (t1.symbol == '/') {
		input_next_token(lexer->input, &t1, &t2);
		if(t2.symbol == '*') {
			tn = t2;
			while(1) {
				int prev = tn.symbol;
				input_next_token(lexer->input, &tn, &tn);
				if(tn.symbol == '/' && prev == '*') {
					break;
				}
			}
			token_init(t_out, t1.index, tn.index - t1.index + 1, ATL_COMMENT);
		} else {
			*t_out = t1;
		}
	} else {
		*t_out = t1;
	}

	if(t_out->symbol == ATL_WHITE_SPACE || t_out->symbol == ATL_COMMENT) {
		*t_in = *t_out;
		goto next_token;
	}

	return;
eof:
	token_init(t_out, t1.index, 0, L_EOF);
}

