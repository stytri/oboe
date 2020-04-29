#ifndef TEST_TIMING_H
#define TEST_TIMING_H 1

#include <stddef.h>
#include <stdio.h>
#include <time.h>

static char const *
get_units(
	double *valuep
) {
	char const *units = "";

	if(*valuep < 1.0) {
		*valuep *= 1000.0;
		units = "milli";
	}
	if(*valuep < 1.0) {
		*valuep *= 1000.0;
		units = "micro";
	}
	if(*valuep < 1.0) {
		*valuep *= 1000.0;
		units = "nano";
	}
	if(*valuep < 1.0) {
		*valuep *= 1000.0;
		units = "pico";
	}

	return units;
}

static void
print_timings(
	size_t      N,
	char const *operations,
	clock_t     start,
	clock_t     end
) {
	double      delta       = (double)(end - start) / CLOCKS_PER_SEC;
	double      per         = delta / (double)N;
	char const *delta_units = get_units(&delta);
	char const *per_units   = get_units(&per);

	printf("%zu %s in %g %sseconds at a rate of 1 per %g %sseconds\n",
		N,
		operations,
		delta, delta_units,
		per, per_units
	);
}

#endif// TEST_TIMING_H
