#ifndef OUTPUT_H
#define OUTPUT_H

#define OUTPUT_DEFAULT 0
#define OUTPUT_FSM_ERROR 1

typedef struct Output {
	int status;
} Output;

void output_init(Output *output);
void output_dispose(Output *output);
void output_raise(Output *output, int flag);

#endif //OUTPUT_H
