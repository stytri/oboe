#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "src\marray.h"
#include "test_timing.h"

struct array numbers = ARRAY();

int main(
	int   argc,
	char *argv[]
) {
#ifndef NDEBUG
	size_t N          = 1000000000LU;
#else
	size_t N          = 1000000LU;
#endif
	bool   push_back  = true;
	bool   use_array  = false;
	for(int i = 1; argc > i; ++i) {
		char const *args = argv[i];

		if(isdigit(*args)) {
			N = strtoumax(args, NULL, 0);

		} else if(!strcmp(args, "create")) {
			push_back = false;

		} else if(!strcmp(args, "array")) {
			use_array = true;
		}
	}

	clock_t istart, iend, tstart, tend;

	puts("Initializing...");
	if(push_back) {
		if(use_array) {
			istart = clock();
			for(size_t i = 1; i < N; ++i) {
				array_push_back(&numbers, size_t, -i);
			}
			iend = clock();
		} else {
			istart = clock();
			for(size_t i = 1; i < N; ++i) {
				marray_push_back(&numbers, size_t, -i);
			}
			iend = clock();
		}
	} else {
		if(use_array) {
			istart = clock();
			for(size_t i = 1; i < N; ++i) {
				size_t *p = array_create_back(&numbers, size_t);
				*p = -i;
			}
			iend = clock();
		} else {
			istart = clock();
			for(size_t i = 1; i < N; ++i) {
				size_t *p = marray_create_back(&numbers, size_t);
				*p = -i;
			}
			iend = clock();
		}
	}
	print_timings(N, "appends", istart, iend);

	puts("Testing...");
	if(use_array) {
		tstart = clock();
		for(size_t i = 1; i < N; ++i) {
			size_t x = array_at(&numbers, size_t, i-1);
			if(x != -i) {
				printf("[%zi] %zi != %zi\n", i-1, x, -i);
			}
		}
	} else {
		tstart = clock();
		for(size_t i = 1; i < N; ++i) {
			size_t x = marray_at(&numbers, size_t, i-1);
			if(x != -i) {
				printf("[%zi] %zi != %zi\n", i-1, x, -i);
			}
		}
	}
	tend = clock();
	print_timings(N, "readings", tstart, tend);

    return 0;
}
