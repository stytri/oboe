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
#include "eval.h"
#include "env.h"
#include "odt.h"
#include "trace.h"
#include "gc.h"

//------------------------------------------------------------------------------

static Ast
getref(
	Ast      env,
	sloc_t   sloc,
	String   s,
	uint64_t hash
) {
	size_t      n;
	char const *cs  = StringToCharLiteral(s, &n);
	Ast         ref = lookup(env, hash, cs, n, 0);
	if(ast_isnotZen(ref)) {
		return ref;
	}

	return oboerr(sloc, ERR_InvalidIdentifier);
}

//------------------------------------------------------------------------------

Ast
assign(
	sloc_t sloc,
	Ast   *past,
	Ast    expr
) {
	Ast ast = *past;
	if(ast_isAssignable(ast)) {

		if(ast != expr) {
			if(ast_isCopyOnAssign(ast)) {
				*past = ast = new_ast(sloc, AST_Void);
			}

			memcpy(ast, expr, sizeof(*ast));

			if(ast_isRemoveCopyOnAssign(ast)) {
				ast->attr &= ~ATTR_CopyOnAssign;
			}
			ast->attr &= ~ATTR_NoAssign;
			ast->sloc = sloc;
		}

		return ast;
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

Ast
evalop(
	Ast    env,
	sloc_t sloc,
	size_t index,
	Ast    lexpr,
	Ast    rexpr
) {
	Ast ast = getopr(index);

	switch(ast_type(ast)) {
	case AST_BuiltinOperator:
		if(ast->m.bop) {
			ast = ast->m.bop(env, sloc, lexpr, rexpr);
			return ast;
		}
		break;
	case AST_OperatorFunction: {
			Array statics_env = statics->m.env;
			Ast   locals_save = source_env(sloc_source(ast->sloc));
			statics->m.env    = locals_save->m.env;

			locals_save = locals;
			ast = ast->m.rexpr;
			locals = new_env(sloc, env);
			addenv_operands(locals, env, sloc, ast->m.lexpr, lexpr, rexpr);
			ast = refeval(locals, ast->m.rexpr);
			locals = locals_save;

			statics->m.env = statics_env;
			return ast;
		}
	case AST_Error:
		return ast;
	default:
		break;
	}

	return oboerr(sloc, ERR_InvalidOperator);
}

//------------------------------------------------------------------------------

Ast
subeval__actual(
	Ast env,
	Ast ast
) {
	size_t ts = gc_topof_stack();

	for(Ast prev = ZEN; prev != ast; ) {
		TRACE(trace_global_indent,ast);
		switch(ast_type(prev = ast)) {
		default: {
#	define ENUM(Name)  } break; case AST_##Name: {
#	define EVAL(...)    __VA_ARGS__;

#	define RETURN(...)  __VA_ARGS__; goto return_ast
#	define REFERENCE(Ident,Hash)      getref   (env, ast->sloc, Ident, Hash)
#	define OPERATOR(Qual,Lexpr,Rexpr) evalop   (env, ast->sloc, Qual, Lexpr, Rexpr)

#	include "oboe.enum"

#	undef RETURN
#	undef REFERENCE
#	undef OPERATOR
		}}
	}

return_ast:
	return gc_return(ts, ast);
}

Ast
refeval__actual(
	Ast env,
	Ast ast
) {
	for(ast = subeval__actual(env, ast);
		ast_isReference(ast);
		ast = subeval(env, ast->m.rexpr)
	);

	return ast;
}

Ast
eval__actual(
	Ast env,
	Ast ast
) {
	for(ast = refeval__actual(env, ast);
		ast_isQuoted(ast);
		ast = refeval(env, ast->m.rexpr)
	);

	ast->attr |= ATTR_NoEvaluate;
	return ast;
}

Ast
evalseq__actual(
	Ast    env,
	Ast    ast
) {
	Ast result = ZEN;

	size_t ts = gc_topof_stack();
	for(; ast_isSequence(ast); ast = ast->m.rexpr) {
		result = eval(env, ast->m.lexpr);

		gc_return(ts, result);
	}
	if(ast_isnotZen(ast)) {
		result = eval(env, ast);

		gc_return(ts, result);
	}

	return result;
}

Ast
eval_named(
	Ast         env,
	sloc_t      sloc,
	char const *name
) {
	String s = CharLiteralToString(name, strlen(name));
	assert(s != NULL);
	Ast    a = new_ast(sloc, AST_Identifier, s);

	return eval__actual(env, a);
}

