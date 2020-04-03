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
#include "trace.h"
#include "tostr.h"
#include "sources.h"
#include <stdio.h>

//------------------------------------------------------------------------------

void
trace(
	size_t      indent,
	Ast         ast,
	char const *comment
) {
	static const char tabs[] = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
#	define N_TABS (sizeof(tabs) - 1)

	if(indent > 0) {
		for(; indent > N_TABS; indent -= N_TABS) {
			fputs(tabs, stdout);
		}
		if(indent > 0) {
			fputs(&tabs[N_TABS - indent], stdout);
		}
	}

	if(ast_isnotZen(ast)) {
		String      s      = StringCreate();
		String      srcs   = get_source(sloc_source(ast->sloc));
		char const *source = StringToCharLiteral(srcs, NULL);
		char        line[(CHAR_BIT * sizeof(unsigned)) + 1];
		snprintf(line, sizeof(line), ":%u: ", sloc_line(ast->sloc));

		s = StringAppendCharLiteral(s, source , strlen(source));
		s = StringAppendCharLiteral(s, line   , strlen(line));
		s = StringAppend           (s, tostr(ast, true));

		if(s) {
			fputs(StringToCharLiteral(s, NULL), stdout);
			StringDelete(s);
			if(comment && *comment) {
				fputc(' ', stdout);
			}
		}
	}

	if(comment && *comment) {
		fputs("# ", stdout);
		fputs(comment, stdout);
	}

	fputc('\n', stdout);
	fflush(stdout);

	return;
#	undef N_TABS
}

//------------------------------------------------------------------------------

bool   trace_enabled       = false;
size_t trace_global_indent = 0;

