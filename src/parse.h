#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED
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

#include <stddef.h>
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef enum {
	P_None,
	P_Declarative,
	P_Assigning,
	P_Conditional,
	P_Logical,
	P_Relational,
	P_Bitwise,
	P_Additive,
	P_Multiplicative,
	P_Exponential,
	P_Binding

} Precedence;


extern Precedence
precedence(
	char const *cs,
	size_t      n
);

extern Ast
parse(
	char const   *cs,
	char const  **endp,
	unsigned      source,
	unsigned     *linop,
	Ast         (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	),
	bool          all
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef PARSE_H_INCLUDED
