#ifndef EVAL_H_INCLUDED
#define EVAL_H_INCLUDED
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
#include "env.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern Ast
assign(
	sloc_t sloc,
	Ast   *past,
	Ast    expr
);

extern Ast
evalop(
	Ast    env,
	sloc_t sloc,
	size_t index,
	Ast    lexpr,
	Ast    rexpr
);

extern Ast
subeval(
	Ast env,
	Ast ast
);

extern Ast
refeval(
	Ast env,
	Ast ast
);

extern Ast
eval(
	Ast env,
	Ast ast
);

extern Ast
evalseq(
	Ast    env,
	Ast    ast
);

extern Ast
eval_named(
	Ast         env,
	sloc_t      sloc,
	char const *name
);

//------------------------------------------------------------------------------

static inline Ast
unwrapref(
	Ast arg
) {
	for(;
		ast_isReference(arg->m.rexpr);
		arg = arg->m.rexpr
	);
	return arg;
}

static inline Ast
deref(
	Ast arg
) {
	for(;
		ast_isReference(arg);
		arg = arg->m.rexpr
	);
	return arg;
}

static inline Ast
dereference(
	Ast env,
	Ast arg
) {
	if(ast_isIdentifier(arg)) {
		arg = subeval(env, arg);
		arg = deref(arg);
	}
	return arg;
}

static inline Ast
undefer(
	Ast env,
	Ast ast
) {
	if(ast_isIdentifier(ast)) {
		for(ast = subeval(env, ast);
			ast_isDeferred(ast);
			ast = ast->m.rexpr
		);
	}
	return ast;
}

static inline Ast
unquote(
	Ast ast
) {
	for(;
		ast_isQuoted(ast);
		ast = ast->m.rexpr
	);
	return ast;
}

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef EVAL_H_INCLUDED
