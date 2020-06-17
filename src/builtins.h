#ifndef BUILTINS_H_INCLUDED
#define BUILTINS_H_INCLUDED
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
#include "parse.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

struct builtinop {
	char const *leme;
	BuiltinOp   func;
	unsigned   *enup;
	Precedence  prec;
};
extern int
initialise_builtinop(
	Ast                    env,
	struct builtinop const builtinop[],
	size_t                 n_builtinop
);

struct builtinfn {
	char const *leme;
	BuiltinFn   func;
	unsigned   *enup;
};
extern int
initialise_builtinfn(
	Ast                    env,
	struct builtinfn const builtinfn[],
	size_t                 n_builtinfn
);

extern int
initialise_builtin_math_functions(
	Ast env
);

#define BUILTIN(Lexeme, Name, ...)  { Lexeme, builtin_##Name, &builtin_##Name##_enum, __VA_ARGS__ },

//------------------------------------------------------------------------------

extern int
initialise_builtins(
	bool has_math
);

//------------------------------------------------------------------------------

extern String
mapoboefile(
	StringConst file
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef BUILTINS_H_INCLUDED
