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
#include "searchpaths.h"
#include "strlib.h"
#include "assert.h"
#include "hash.h"
#include "env.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

//------------------------------------------------------------------------------

Ast searchpaths = NULL;

//------------------------------------------------------------------------------

int
initialise_searchpaths(
	void
) {
	static bool initialise = true;

	if(initialise) {
		initialise = false;

		searchpaths = new_env(0, NULL);
	}

	return EXIT_SUCCESS;
}

size_t
num_searchpaths(
	void
) {
	return searchpaths ? array_length(searchpaths->m.env) : 0;
}

size_t
add_searchpath(
	sloc_t sloc,
	String s
) {
	assert(searchpaths != NULL);

	size_t ts = gc_topof_stack();

	size_t      n;
	char const *cs = StringToCharLiteral(s, &n);

	if((n > 0) && (cs[n-1] != '/')) {
		String t = DuplicateString(s);
		assert(t != NULL);
		s = StringAppendChar(t, '/');
		assert(s != NULL);
		if(s != t) {
			StringDelete(t);
		}
		cs = StringToCharLiteral(s, &n);
	}

	Ast    ast   = new_ast(sloc, AST_String, s);
	size_t index = locate(searchpaths, ast->m.hash, cs, n);
	if(!~index) {
		index = define(searchpaths, ast->m.hash, ast, ATTR_NoAssign);
		assert(~index != 0);
	}

	gc_revert(ts);

	return index;
}

String
get_searchpath(
	size_t index
) {
	assert(searchpaths != NULL);
	assert(index < array_length(searchpaths->m.env));

	return array_at(searchpaths->m.env, Ast, index)->m.sval;
}

