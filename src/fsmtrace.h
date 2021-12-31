
#ifndef FSMTRACE_H
#define FSMTRACE_H

#include <stdio.h>

#ifdef FSM_TRACE

#define trace_state(M, S, A) \
	printf( \
		"%-5s: %-9p(refstat:%i) %-13s\n", \
		(M), (S), (S)->status, (A) \
	)
#define trace_match(S, Y, P, C) \
	printf( \
		"match: %-9p (%3i =%2c) path=%i count=%i\n", \
		(S), (Y), (Y), (P), (C) \
	)
#define trace_symbol(M, S) \
	printf("trace: %-5s: %.*s [id:%i]\n", M, (S)->length, (S)->name, (S)->id);

#define trace_op(M, T1, T2, S, A, R) \
	printf( \
		"%-5s: [%-9p --(%-9p:%i)--> %-9p] %-13s %c %3i (%3i =%2c)\n", \
		M, \
		T1, \
		T2, \
		T2? ((Action*)T2)->flags: 0, \
		T2? ((Action*)T2)->state: NULL, \
		A, \
		(R != 0)? '>': ' ', \
		R, \
		S, (char)S \
	)

#else

#define trace_state(M, S, A)
#define trace_match(S, Y, P, C)
#define trace_symbol(M, S)
#define trace_op(M, T1, T2, S, A, R)

#endif

#endif //FSMTRACE_H
