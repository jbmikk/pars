#include "listener.h"

void listener_init(Listener *listener, ListenerFunction *function, void *object)
{
	listener->object = object;
	listener->function = function;
}

int listener_notify(Listener *listener, void *params)
{
	int ret = 0;
	if(listener->function) {
		ret = listener->function(listener->object, params);
	}
	return ret;
}

