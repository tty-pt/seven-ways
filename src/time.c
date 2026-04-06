#include "./../include/time.h"
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

double time_tick;

static long long start_t;
static long long
timestamp(void)
{
	struct timeval te;
	gettimeofday(&te, NULL); // get current time
	return te.tv_sec * 1000000LL + te.tv_usec;
}

void
time_init(void)
{
	start_t = timestamp();
	time_tick = 0;
}

double
dt_get(void)
{
	long long tick = timestamp();
	double ret = (tick - start_t) / 1000000.0;
	start_t = tick;
	time_tick += ret;
	return ret;
}
