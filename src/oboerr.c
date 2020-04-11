#include "ast.h"
#include <stdio.h>
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

//------------------------------------------------------------------------------

static struct {
	char const *message;
} oboe_errors[] = {
#	define ENUM(Name,...) { #Name },
#	include "oboerr.enum"
};

//------------------------------------------------------------------------------

Ast
oboerr(
	sloc_t sloc,
	Error  err
) {
	return new_ast(sloc, NULL, AST_Error, err);
}

char const *
oboerrs(
	Error err
) {
	if((0 <= err) && (err < N_Errors)) {
		return oboe_errors[err].message;
	}

	static char message[((CHAR_BIT * sizeof(unsigned)) / 4) + 1];

	snprintf(message, sizeof(message), "%x", (unsigned)err);

	return message;
}
