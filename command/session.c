#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session.h"

static int execute_fully_off(void *context);
static int execute_fully_on(void *context);
static int execute_brightness(void *context);
static int execute_blink(void *context);
static int execute_time(void *context);
static int execute_wave(void *context);

static int broker_push(session_t *s);
static session_t *broker_pop(void);
static int broker_schedule(void);

static session_base_t session_maps[] = {
	{SESSION_FULLY_OFF, 	execute_fully_off},
	{SESSION_FULLY_ON, 		execute_fully_on},
	{SESSION_BRIGHTNESS, 	execute_brightness},
	{SESSION_BLINK, 		execute_blink},
	{SESSION_TIME, 			execute_time},
	{SESSION_WAVE, 			execute_wave},
};
#define SESSION_MAP_COUNT 	(sizeof(session_maps)/sizeof(session_maps[0]))

session_broker_t session_broker = {
	.push = broker_push,
	.pop = broker_pop,
	.schedule = broker_schedule,
};

static exp_link_t exp_links[] = {
	{"Fully On", 	8, SESSION_FULLY_ON},
	{"Fully Off", 	9, SESSION_FULLY_OFF},
	{"Brightness", 10, SESSION_BRIGHTNESS},
	{"Blink", 		5, SESSION_BLINK},
	{"Show Time", 	9, SESSION_TIME},
	{"Show Wave", 	9, SESSION_WAVE},
};
#define LINK_COUNT 	(sizeof(exp_links)/sizeof(exp_links[0]))


static int execute_fully_off(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static int execute_fully_on(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static int execute_brightness(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static int execute_blink(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static int execute_time(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static int execute_wave(void *context)
{
	printf("%s %p\n", __FUNCTION__, context);

	return 0;
}

static session_base_t *get_session_base(session_type_t type)
{
	session_base_t *s;

	for (s = session_maps; s < (session_maps + SESSION_MAP_COUNT); s++) {
		if (s->type == type)
			return s;
	}

	return NULL;
}

static int broker_push(session_t *session)
{
	session_broker_t *broker = &session_broker;
	session_t *node;

	printf("%s %p\n", __FUNCTION__, session);

	if (!session) {
		fprintf(stderr, "%s null session\n", __FUNCTION__);
		return -1;
	}

	node = broker->sessions;
	if (!node) {
		broker->sessions = session;
		return 0;
	}
	else {
		for (; node; node = node->next) {
			if (!node->next) {
				node->next = session;
				return 0;
			}
		}

		fprintf(stderr, "%s should not be here !\n", __FUNCTION__);
		return -1;
	}
}

static session_t *broker_pop(void)
{
	session_broker_t *broker = &session_broker;
	session_t *node = broker->sessions;

	printf("%s %p\n", __FUNCTION__, node);

	if (node) {
		broker->sessions = node->next;
		return node;
	}

	return NULL;
}

static int broker_schedule(void)
{
	session_t *node;

	while ((node = broker_pop()) != NULL) {
		if (node->base) {
			node->base->execute(node->extra);
		}

		session_del(node);
	}

	return 0;
}

static exp_link_t *get_link(const char *exp)
{
	exp_link_t *link;

	for (link = exp_links; link < (exp_links + LINK_COUNT); link++) {
		if (strncmp(link->key, exp, link->len) == 0)
			return link;
	}

	return NULL;
}

static session_t *_session_new(session_type_t type, void *extra)
{
	session_t *self;

	self = malloc(sizeof(*self));
	if (!self) {
		printf("%s malloc\n", __FUNCTION__);
		return NULL;
	}

	self->base = get_session_base(type);
	self->extra = extra;
	self->next = NULL;
	return self;
}

session_t *session_new(const char *exp)
{
    session_t* self;
    exp_link_t *link;
    void *extra = NULL;
    int recycle = 0;

    if (!exp) {
    	fprintf(stderr, "[SESSION] null exp\n");
    	return NULL;
    }

    link = get_link(exp);
    if (!link) {
    	fprintf(stderr, "[SESSION] unrecognized exp - %s\n", exp);
    	return NULL;
    }

    // process extra data
    switch (link->type) {
    case SESSION_BRIGHTNESS: {
    		char *er = (char *)exp + link->len;
    		int br;

    		if (sscanf(er, "%d", &br) != 1) {
    			fprintf(stderr, "[SESSION] invalid exp - %s\n", exp);
    			return NULL;
    		}
    		else {
    			extra = (void *)br;
    		}
    	}
    	break;

    case SESSION_BLINK: {
    		char *er = (char *)exp + link->len;

    		// discard character ' '
    		extra = strlen(er) > 1 ? strdup(er+1) : NULL;
    		if (!extra) {
    			fprintf(stderr, "[SESSION] strdup? invalid exp - %s\n", exp);
    			return NULL;
    		}

    		// free flag
    		recycle = 1;
    	}
    	break;

    default:
    	break;
    }

    self = _session_new(link->type, extra);
    if (!self) {
    	fprintf(stderr, "[SESSION] null session\n");

    	if (recycle)
    		free(extra);

    	return NULL;
    }

    return self;
}

void session_del(session_t *self)
{
	session_base_t *base;

	if (!self)
		return;

	base = self->base;
	if (!base)
		return;

	switch (base->type) {
    case SESSION_BLINK:
	    if (self->extra)
	    	free(self->extra);
    	break;

    default:
    	break;
    }

	free(self);
}
