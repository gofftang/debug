#include "subject.h"

static void observer_test(void)
{
	subject_t *subject = &g_subject;

	subject->set_state(5);
	subject->set_state(10);
}

int main(int argc, char *argv[])
{
	observer_test();

	return 0;
}