#include "output.h"

void output_init(Output *output)
{
	output->status = OUTPUT_DEFAULT;
}

void output_dispose(Output *output)
{
}

void output_raise(Output *output, int flag)
{
	output->status |= flag;
}

