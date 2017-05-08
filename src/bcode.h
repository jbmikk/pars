#ifndef BCODE_H
#define BCODE_H

typedef struct _Instruction {
	char opcode;
	char size;
} Instruction;

typedef struct _Bcode {
	unsigned int size;
	unsigned int assigned;
	void *frame;
} Bcode;

void bcode_init(Bcode *bcode);
void bcode_dispose(Bcode *bcode);

void bcode_add_instruction(Bcode *bcode, Instruction *instruction);

#endif //BCODE_H
