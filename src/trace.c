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
	static unsigned last_source = ~0u;
	static unsigned last_line   = ~0u;

	if(indent > 0) {
		for(; indent > N_TABS; indent -= N_TABS) {
			fputs(tabs, stdout);
		}
		if(indent > 0) {
			fputs(&tabs[N_TABS - indent], stdout);
		}
	}

	bool do_trace = true;

	if(ast_isnotZen(ast)) {
		unsigned this_source = sloc_source(ast->sloc);
		unsigned this_line   = sloc_line  (ast->sloc);

		do_trace = trace_verbose
			|| (last_source != this_source)
			|| (last_line   != this_line)
		;
		if(do_trace) {
			String      s      = StringCreate();
			String      srcs   = get_source(this_source);
			char const *source = StringToCharLiteral(srcs, NULL);
			char        line[(CHAR_BIT * sizeof(unsigned)) + 1];
			if(trace_verbose) {
				snprintf(line, sizeof(line), ":%u: ", this_line);
			} else {
				snprintf(line, sizeof(line), ":%u", this_line);
			}

			s = StringAppendCharLiteral(s, source , strlen(source));
			s = StringAppendCharLiteral(s, line   , strlen(line));
			if(trace_verbose) {
				s = StringAppend(s, tostr(ast, true));
			}

			if(s) {
				fputs(StringToCharLiteral(s, NULL), stdout);
				StringDelete(s);
				if(comment && *comment) {
					fputc(' ', stdout);
				}
			}

			last_source = this_source;
			last_line   = this_line;
		}
	}

	if(do_trace) {
		if(comment && *comment) {
			fputs("# ", stdout);
			fputs(comment, stdout);
		}

		fputc('\n', stdout);
		fflush(stdout);
	}

	return;
#	undef N_TABS
}

//------------------------------------------------------------------------------

bool   trace_enabled       = false;
bool   trace_verbose       = false;
size_t trace_global_indent = 0;

