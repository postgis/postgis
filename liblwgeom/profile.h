#include "liblwgeom.h"
#include <time.h>

static clock_t start_time, end_time;
static double cpu_time_used;

/*
 * A convenience macro for performance testing and breaking
 * down what time is spent where.
 */

#define START_PROFILING() do { start_time = clock(); } while(0)
#define END_PROFILING(name) do { \
	end_time = clock(); \
	cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC; \
	lwnotice("Time used for " #name ": %f secs\n", cpu_time_used); \
} while(0)
