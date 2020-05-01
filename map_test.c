#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "src\array.h"
#include "src\mapfile.h"
#include "src\string.h"
#include "src\hash.h"

static int (*is_ctype)(int c) = isalpha;

static int cmp(
	Array       arr,
	size_t      index,
	void const *key,
	size_t      n
) {
	char const *cs = array_at(arr, char const *, index);
	char const *ct = key;
	int         sc = *cs;
	int         tc = *ct;
	for(;
		n && is_ctype(sc) && is_ctype(tc) && (sc == tc);
		n--, sc = *++cs, tc = *++ct
	)
		;
	if(n == 0) {
		return is_ctype(sc) || is_ctype(tc);
	}

	return 0;
}

int main(
	int   argc,
	char *argv[]
) {
	struct array arr = ARRAY();

	bool index = false;
	bool quiet = false;
	bool hash  = false;

	for(int i = 1; i < argc; ++i) {
		if(!strcmp(argv[i], "--alpha")) {
			is_ctype = isalpha;
			continue;
		}
		if(!strcmp(argv[i], "--alnum")) {
			is_ctype = isalnum;
			continue;
		}
		if(!strcmp(argv[i], "--graph")) {
			is_ctype = isgraph;
			continue;
		}
		if(!strcmp(argv[i], "--index")) {
			index = true;
			continue;
		}
		if(!strcmp(argv[i], "--quiet")) {
			quiet = true;
			continue;
		}
		if(!strcmp(argv[i], "--hash")) {
			hash = true;
			continue;
		}

		String s = mapfile(argv[i]);
		if(!s) {
			fflush(stdout);
			fprintf(stderr, "file error: %s\n", strerror(errno));
			fflush(stderr);
			continue;
		}

		char const *cs = StringToCharLiteral(s, NULL);
		if(!cs) {
			StringDelete(s);
			continue;
		}

		char const *start;
		char const *end;

		if(!quiet) {
			puts("Initializing...");
		}
		for(start = cs; *start; start = end) {
			for(; *start && !is_ctype(*start); start++)
				;
			for(end = start; *end && is_ctype(*end); end++)
				;
			if(end > start) {
				size_t   n = end - start;
				uint64_t h = memhash(start, n, 0);
				size_t   x = array_get_index(&arr, h, cmp, start, n);
				if(!~x) {
					x = array_length(&arr);
					if(array_push_back(&arr, char const *, start)) {
						array_map_index(&arr, h, x);
					}
				}
			}
		}

		if(!quiet) {
			puts("Testing...");
		}
		for(start = cs; *start; start = end) {
			for(; *start && !is_ctype(*start); start++)
				;
			for(end = start; *end && is_ctype(*end); end++)
				;
			if(end > start) {
				size_t   n = end - start;
				uint64_t h = memhash(start, n, 0);
				size_t   x = array_get_index(&arr, h, cmp, start, n);
				if(!~x) {
					fprintf(stderr, "NOT FOUND: %*.*s\n", (int)n, (int)n, start);
					continue;
				}
				if(quiet) {
					continue;
				}
				if(index) {
					printf("[%zu]: ", x);
				}
				if(hash) {
					printf("#%16.16llx: ", h);
				}
				printf("%*.*s\n", (int)n, (int)n, array_at(&arr, char const *, x));
			}
		}
	}
}