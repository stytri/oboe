/*
MIT License

Copyright (c) 2019 Tristan Styles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "odt.h"
#include "array.h"
#include "hash.h"
#include "gc.h"

//------------------------------------------------------------------------------

typedef struct odt *Odt;
struct odt {
	char const *name;
	Ast       (*new)(
		Ast     ast,
		va_list va
	);
	Ast       (*eval)(
		Ast ast
	);
	void      (*mark)(
		Ast    ast,
		void (*gc_mark)(void const *)
	);
	void      (*sweep)(
		Ast ast
	);
};

struct array odt_map = ARRAY();

//------------------------------------------------------------------------------

static int
cmp(
	Array       arr,
	size_t      index,
	void const *key,
	size_t      n
) {
	Odt odt = marray_ptr(arr, struct odt, index);
	return strcmp(odt->name, key);
	(void)n;
}

static inline size_t
lookup_odt(
	char const *name,
	uint64_t    hash
) {
	return marray_get_index(&odt_map, hash, cmp, name, 0);
}

static inline Odt
odt_of(
	Ast ast
) {
	if(ast_isnotZen(ast)) {
		size_t index = ast->qual;
		if(index < marray_length(&odt_map)) {
			return marray_ptr(&odt_map, struct odt, index);
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

unsigned
add_odt(
	char const *name,
	Ast       (*new)(
		Ast     ast,
		va_list va
	),
	Ast       (*eval)(
		Ast ast
	),
	void      (*mark)(
		Ast    ast,
		void (*gc_mark)(void const *)
	),
	void      (*sweep)(
		Ast ast
	)
) {
	size_t   len   = strlen(name);
	uint64_t hash  = memhash(name, len, 0);
	size_t   index = lookup_odt(name, hash);

	if(!~index) {
		struct odt const odt = {
			name,
			new,
			eval,
			mark,
			sweep
		};
		size_t next_index = marray_length(&odt_map);
		if(marray_push_back(&odt_map, struct odt, odt)) {
			index = marray_map_index(&odt_map, hash, next_index);
		}
	}

	return (unsigned)index;
}

unsigned
get_odt(
	char const *name
) {
	size_t   len   = strlen(name);
	uint64_t hash  = memhash(name, len, 0);
	size_t   index = lookup_odt(name, hash);

	return (unsigned)index;
}

char const *
odt_name(
	Ast ast
) {
	Odt odt = odt_of(ast);
	return odt ? odt->name : "";
}

Ast
odt_new(
	Ast     ast,
	va_list va
) {
	ast->qual = va_arg(va, unsigned);

	Odt odt = odt_of(ast);
	return odt ? odt->new(ast, va) : ast;
}

Ast
odt_eval(
	Ast ast
) {
	Odt odt = odt_of(ast);
	return odt ? odt->eval(ast) : ast;
}

void
odt_mark(
	Ast    ast,
	void (*gc_mark)(void const *)
) {
	Odt odt = odt_of(ast);
	if(odt) odt->mark(ast, gc_mark);
}

void
odt_sweep(
	Ast ast
) {
	Odt odt = odt_of(ast);
	if(odt) odt->sweep(ast);
}

