#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "src\assert.h"
#include "src\array.h"
#include "src\mapfile.h"
#include "src\string.h"
#include "src\hash.h"

static int
isnoteol(
	int c
) {
	return c
		&& (c != '\f')
		&& (c != '\n')
		&& (c != '\r')
		&& (c != '\v')
		;
}

static int (*is_ctype)(int c) = isnoteol;

struct entry {
	char const *cs;
	size_t      len;
};

#define ENTRY_SLAB_LEN ((1024LU * 1024LU / sizeof(struct entry)) - 1)

struct entry_slab {
	struct entry_slab *prev;
	size_t             count;
	struct entry       entry[ENTRY_SLAB_LEN];
}	_entry_slab = { NULL, 0, {{ NULL, 0 }} },
	*entry_slab = &_entry_slab;

static struct entry *
new_entry(
	char const *cs,
	size_t      len
) {
	if(entry_slab->count == ENTRY_SLAB_LEN) {
		struct entry_slab *new_slab = malloc(sizeof(*new_slab));
		assert(new_slab != NULL);

		new_slab->prev  = entry_slab;
		new_slab->count = 0;
		entry_slab      = new_slab;
	}

	struct entry *new_entry = &entry_slab->entry[entry_slab->count++];

	new_entry->cs  = cs;
	new_entry->len = len;

	return new_entry;
}

static int cmp(
	Array       arr,
	size_t      index,
	void const *key,
	size_t      n
) {
	struct entry *entry = array_at(arr, struct entry *, index);
	if(n == entry->len) {

		char const *cs = entry->cs;
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
	}

	return 0;
}

static size_t collision     = 0;
static size_t ncollisions   = 0;
static size_t maxcollisions = 0;

static int map_print_ndigits = 9;

static int
map_print(
	void    *context,
	size_t   index,
	uint64_t hash
) {
	static uint64_t last_hash = 0;

	struct entry *entry = array_at(context, struct entry *, index);

	if(last_hash != hash) {
		last_hash = hash;
		printf("#%16.16llx", hash);
		collision = 0;
	} else {
		printf("+%16.16s", "");
		collision++;
		ncollisions++;
		if(maxcollisions < collision) {
			maxcollisions = collision;
		}
	}
	int n = entry->len;
	printf("[%*zu] %*.*s\n", map_print_ndigits, index, n, n, entry->cs);

	return 0;
	(void)context;
}

static int
ndigits(
	uintmax_t n
) {
	int d = 1;

	for(; n > 999999999LLU; n /= 1000000000LLU, d += 9);
	for(; n > 999999LU; n /= 1000000LU, d += 6);
	for(; n > 999LU; n /= 1000LU, d += 3);
	for(; n > 9LU; n /= 10LU, d += 1);

	return d;
}

static void
map(
	Array arr
) {
	map_print_ndigits = ndigits(array_length(arr));

	array_foreach(arr, map_print, arr);
}

int main(
	int   argc,
	char *argv[]
) {
	struct array arr = ARRAY();

	bool verbose = false;
	bool index   = false;
	bool quiet   = false;
	bool hash    = false;

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
		if(!strcmp(argv[i], "--verbose")) {
			verbose = true;
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
		if(!strcmp(argv[i], "--map")) {
			map(&arr);
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

		if(verbose) {
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
					if(array_push_back(&arr, struct entry *, new_entry(start, n))) {
						array_map_index(&arr, h, x);
					}
				}
			}
		}

		if(verbose) {
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
				printf("%*.*s\n", (int)n, (int)n, array_at(&arr, struct entry *, x)->cs);
			}
		}

		if(verbose) {
			if(ncollisions > 0) {
				printf("..Finished: collisions %zu : %zu\n", ncollisions, maxcollisions);
			} else {
				puts("..Finished");
			}
		}
	}
}
