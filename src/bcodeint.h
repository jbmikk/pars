#ifndef BCODEINT_H
#define BCODEINT_H

#include "bcode.h"

typedef struct _Bcodeint {
	Bcode bcode;
	unsigned int ip;
	AstCursor cursor;
} Bcodeint;

void bcodeint_init(Bcodeint *bcodeint);
void bcodeint_dispose(Bcodeint *bcodeint);

void bcodeint_run(Bcodeint *bcodeint);

#endif //BCODEINT_H
