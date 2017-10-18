#ifndef LISTENER_H
#define LISTENER_H

typedef int (ListenerFunction)(void *object, void *params);

typedef struct Listener {
	void *object;
	ListenerFunction *function;
} Listener;

//TODO: Maybe have a type for listener arguments?
//typedef struct Args {
//} Args;

void listener_init(Listener *listener, ListenerFunction *function, void *object);
int listener_notify(Listener *listener, void *params);

#endif //LISTENER_H
