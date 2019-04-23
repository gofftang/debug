#ifndef _VIEW_STUDENT_H_
#define _VIEW_STUDENT_H_

typedef struct view_student_tag {
	void (*update)(char *name, int roll_no);
} view_student;

typedef struct observer {
	void *subject;
	void (*update)(void);
} observer_t;

#endif /* _VIEW_STUDENT_H_ */
