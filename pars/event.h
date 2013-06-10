#ifndef EVENT_H
#define EVENT_H

#define EVENT_HANDLER(N) int (*N)(void *target, void *args)

typedef struct _EventListener {
	void *target;
	EVENT_HANDLER(handler);
} EventListener;

#define TRIGGER(L, A) ((L).handler(L.target, A))
#define TRY_TRIGGER(L, A) ((L).handler && (L).handler(L.target, A))

#endif //EVENT_H
