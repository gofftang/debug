#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

typedef struct controller {
	void *model;
	void *view;

	void (*set_name)(char *name);
	char *(*get_name)(void);

	void (*draw)(void);
} controller_t;

controller_student c_rose;

#endif /* _CONTROLLER_H_ */
