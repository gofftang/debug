#include "observer.h"
#include "subject.h"

#include <stdio.h>

static void dot_update(void);
static void line_update(void);
static void surface_update(void);

observer_t dot_observer = {
	.subject = &g_subject,
	.update = dot_update,
};

observer_t line_observer = {
	.subject = &g_subject,
	.update = line_update,
};

observer_t surface_observer = {
	.subject = &g_subject,
	.update = surface_update,
};


static void dot_update(void)
{
	observer_t *obs = &dot_observer;
	subject_t *sub = obs->subject;

	printf("%s %d\n", __FUNCTION__, sub->get_state());
}

static void line_update(void)
{
	observer_t *obs = &dot_observer;
	subject_t *sub = obs->subject;

	printf("%s %d\n", __FUNCTION__, sub->get_state());
}

static void surface_update(void)
{
	observer_t *obs = &dot_observer;
	subject_t *sub = obs->subject;

	printf("%s %d\n", __FUNCTION__, sub->get_state());
}
