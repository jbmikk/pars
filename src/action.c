#include "fsm.h"

void action_init(Action *action, char type, int reduction, State *state, char flags, int end_symbol)
{
	action->type = type;
	action->reduction = reduction;
	action->state = state;
	action->flags = flags;
	action->end_symbol = end_symbol;
	action->mode = 0;
}

int action_compare(Action a1, Action a2)
{
	return a1.type == a2.type &&
		a1.state == a2.state &&
		a1.reduction == a2.reduction &&
		a1.end_symbol == a2.end_symbol &&
		a1.flags == a2.flags? 0: 1;
}

