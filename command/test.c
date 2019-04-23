#include "session.h"
#include <stdio.h>

static void session_test(void)
{
	const char *exps[] = {
		"Fully On",
		"Fully Off",
		"Brightness 15",
		"Blink 0.5Hz",
		"Show Time",
		"Show Wave",
		NULL
	}, **exp = exps;
	session_broker_t *broker;

	broker = &session_broker;

	for (exp = exps; *exp; exp++) {
		broker->push(session_new((const char *)*exp));
	}

	broker->schedule();
}

int main(int argc, char *argv[])
{
	session_test();

	return 0;
}