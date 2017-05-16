#ifndef BCODE_H
#define BCODE_H

#define OPCODE_SELECT 1
#define OPCODE_EACH 2
#define OPCODE_END 3
#define OPCODE_CALL 4

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
Instruction *bcode_get_instruction(Bcode *bcode, unsigned int index);

#endif //BCODE_H
