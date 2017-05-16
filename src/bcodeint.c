
#include "bcodeint.h"

void bcodeint_init(Bcodeint *bcodeint, Bcode *bcode)
{
	bcodeint->bcode = bcode;
	bcodeint->ip = 0;
}

void bcodeint_dispose(Bcodeint *bcodeint)
{
}

void bcodeint_run(Bcodeint *bcodeint)
{
	while(bcodeint->ip < bcode->size) {
		Instruction *inst = bcode_get_instruction(bcode, bcodeint->ip);
		
		bcodeint->ip += sizeof(Instruction);

		switch(inst->opcode) {
		case OPCODE_SELECT:
			int size = inst->size;
			while(size > 0) {
				int *symbol = bcode_get_int(bcode, bcodeint->ip);
				bcodeint->ip += sizeof(int);
				size--;

				//TODO: What if it fails?
				ast_cursor_depth_next_symbol(bcodeint->cursor, symbol);
			}
			break;
		case OPCODE_EACH:
			ast_cursor_push(&bcodeint->cursor);
			break;
		case OPCODE_END:
			ast_cursor_pop(&bcodeint->cursor);
			break;
		case OPCODE_CALL:
			int *symbol = bcode_get_int(bcode, bcodeint->ip);
			bcodeint->ip += sizeof(int);

			//ebnf_build_syntax_rule(&builder, &cur);
			bcodeint_push_int(bcodeint, bcodeint->ip);
			bcodeint->ip = bcode_get_label_index(bcode, symbol);
			break;
		case OPCODE_RETURN:
			bcodeint->ip = bcodeint_pop_int(bcodeint);
			break;
		case OPCODE_EXIT:
			goto END;
			break;
		}
	}
END:
	return;
}
