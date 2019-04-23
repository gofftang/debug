#ifndef _SUBJECT_H_
#define _SUBJECT_H_

#include "observer.h"

#define OBSERVERS_MAX 	4

struct subject {
	struct observer *observers[OBSERVERS_MAX];

	int state;

	int (*get_state)(void);
	void (*set_state)(int state);

	void (*attach)(struct observer *obs);
	void (*detach)(struct observer *obs);

	void (*notify_all_observers)(void);
};

typedef struct subject subject_t;

subject_t g_subject;

#endif /* _SUBJECT_H_ */