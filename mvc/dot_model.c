#include <string.h>
#include "model.h"

static int dot_set(int x, int y, int color);
static int dot_get(int x, int y);

dot_model_t dot_model = {
	.data = {0},
	.set = dot_set,
	.get = dot_get,
};

static int dot_set(int x, int y, int color)
{
	printf("%s (%d,%d)=%d\n", __FUNCTION__, x, y, color);
	return 0;
}

static int dot_get(int x, int y)
{
	printf("%s (%d,%d)\n", __FUNCTION__, x, y);

	return 0;
}
