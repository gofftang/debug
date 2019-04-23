#include "view_student.h"
#include "stdio.h"

static void update(char *name, int roll_no);

view_student v_rose = {
	.update = update,
};

static void update(char *name, int roll_no)
{
	// view_student *view = &v_rose;

	printf("name: %s, roll_no: %d\n", name, roll_no);
}
