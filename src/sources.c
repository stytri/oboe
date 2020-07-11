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
#include "sources.h"
#include "strlib.h"
#include "assert.h"
#include "hash.h"
#include "env.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

//------------------------------------------------------------------------------

Ast sources = NULL;

//------------------------------------------------------------------------------

int
initialise_sources(
	void
) {
	static bool initialise = true;

	if(initialise) {
		initialise = false;

		sources = new_env(0, NULL);
		String s = CharLiteralToString("<>", 2);
		assert(s != NULL);
		add_source(0, s);
	}

	return EXIT_SUCCESS;
}

unsigned long
add_source(
	sloc_t sloc,
	String s
) {
	assert(sources != NULL);

	size_t ts = gc_topof_stack();

	size_t      n;
	char const *cs    = StringToCharLiteral(s, &n);
	Ast         ast   = new_ast(sloc, AST_String, s);
	size_t      index = locate(sources, ast->m.hash, cs, n);
	if(!~index) {
		index = define(sources, ast->m.hash, ast, ATTR_NoAssign);
		assert(~index != 0 && (index < ULONG_MAX));
	}

	gc_revert(ts);

	return (unsigned long)index;
}

String
get_source(
	size_t index
) {
	assert(sources != NULL);
	assert(index < marray_length(sources->m.env));

	return marray_at(sources->m.env, Ast, index)->m.sval;
}

