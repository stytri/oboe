#ifndef ODT_H_INCLUDED
#define ODT_H_INCLUDED
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

#include "ast.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern unsigned
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
		Ast     ast,
		void  (*gc_mark)(void const *)
	),
	void      (*sweep)(
		Ast ast
	)
);

extern unsigned
get_odt(
	char const *name
);

extern char const *
odt_name(
	Ast ast
);

extern Ast
odt_new(
	Ast     ast,
	va_list va
);

extern Ast
odt_eval(
	Ast ast
);

extern void
odt_mark(
	Ast    ast,
	void (*gc_mark)(void const *)
);

extern void
odt_sweep(
	Ast ast
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef ODT_H_INCLUDED
