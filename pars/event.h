#ifndef EVENT_H
#define EVENT_H

#define EVENT_HANDLER(N) int (*N)(int type, void *target, void *args)

typedef struct _EventListener {
	void *target;
	EVENT_HANDLER(handler);
} EventListener;

#define TRIGGER(T, L, A) ((L).handler(T, L.target, A))
#define TRY_TRIGGER(T, L, A) ((L).handler && (L).handler(T, L.target, A))

#endif //EVENT_H
