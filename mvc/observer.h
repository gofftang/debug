#ifndef _OBSERVER_H_
#define _OBSERVER_H_

#include "subject.h"

struct observer {
	struct subject *subject;

	void (*update)(void);
};

typedef struct observer observer_t;

observer_t dot_observer;
observer_t line_observer;
observer_t surface_observer;

#endif /* _OBSERVER_H_ */