#include "string.h"
#include "subject.h"
#include "observer.h"

static int get_state(void);
static void set_state(int state);
static void attach(observer_t *obs);
static void detach(observer_t *obs);
static void notify_all_observers(void);

subject_t g_subject = {
	.observers = {
		&dot_observer,
		&line_observer,
		&surface_observer,
	},
	.state = 0,
	.get_state = get_state,
	.set_state = set_state,
	.attach = attach,
	.detach = detach,
	.notify_all_observers = notify_all_observers,
};

static int get_state(void)
{
	subject_t *sub = &g_subject;

	return sub->state;
}

static void set_state(int state)
{
	subject_t *sub = &g_subject;

	sub->state = state;
	notify_all_observers();
}

static void attach(observer_t *obs)
{
	subject_t *sub = &g_subject;
	int i;

	for (i = 0; i < OBSERVERS_MAX; i++) {
		if (!sub->observers[i]) {
			sub->observers[i] = obs;
			break;
		}
	}
}

static void detach(observer_t *obs)
{
	subject_t *sub = &g_subject;
	int i;

	for (i = 0; i < OBSERVERS_MAX; i++) {
		if (sub->observers[i] == obs) {
			sub->observers[i] = NULL;
			break;
		}
	}
}

static void notify_all_observers(void)
{
	subject_t *sub = &g_subject;
	int i;

	for (i = 0; i < OBSERVERS_MAX; i++) {
		if (!sub->observers[i])
			continue;

		sub->observers[i]->update();
	}
}
