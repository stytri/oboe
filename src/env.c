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
#include "env.h"
#include "ast.h"
#include "eval.h"
#include "builtins.h"
#include "strlib.h"
#include "assert.h"
#include "gc.h"
#include "hash.h"
#include <stdlib.h>
#include <stdarg.h>

//------------------------------------------------------------------------------

struct env_stats env_stats;

//------------------------------------------------------------------------------

void
env_gc_mark(
	void const *p,
	void      (*mark)(void const *)
) {
	Array env = (Array)p;
	for(size_t i = marray_length(env); i-- > 0; mark(marray_at(env, Ast, i)))
		;
	return;
}

void
env_gc_sweep(
	void const *p
) {
	Array env = (Array)p;
	marray_free(env);
	memset(env, 0, sizeof(*env));
	gc_free(p);
	return;
}

//------------------------------------------------------------------------------

Ast
new_env(
	sloc_t sloc,
	Ast    outer
) {
	Array env = gc_malloc(sizeof(*env), env_gc_mark, env_gc_sweep);
	assert(env != NULL);
	*env = ARRAY();

	Ast ast = new_ast(sloc, AST_Environment, env, outer);
	return ast;
}

Array
dup_env(
	sloc_t sloc,
	Array  env
) {
	Array new_env = gc_malloc(sizeof(*env), env_gc_mark, env_gc_sweep);
	assert(new_env != NULL);
	*new_env = ARRAY();

	size_t n        = marray_length(env);
	bool   expanded = marray_expand(new_env, sizeof(Ast), n);
	assert(expanded);
	new_env->length = n;

	for(size_t i = 0; i < n; i++) {
		Ast ent = marray_at(env, Ast, i);
		ent     = dup_ast(sloc, ent);
		marray_at(new_env, Ast, i) = ent;
		if(ast_isReference(ent)) {
			size_t      len;
			char const *cs = StringToCharLiteral(ent->m.sval, &len);
			uint64_t    h  = memhash(cs, len, 0);
			size_t      x  = marray_map_index(new_env, h, i);
			assert(x == i);
		}
	}

	return new_env;
}

Ast
link_env(
	sloc_t sloc,
	Ast    ast,
	Ast    outer
) {
	return new_ast(sloc, AST_Environment, ast->m.env, outer);
}

//------------------------------------------------------------------------------

static int
cmp(
	Array       arr,
	size_t      index,
	void const *key,
	size_t      n
) {
	Ast ast = marray_at(arr, Ast, index);
	return !StringEqualCharLiteral(ast->m.sval, key, n);
}

//------------------------------------------------------------------------------

size_t
locate(
	Ast         env,
	uint64_t    hash,
	char const *leme,
	size_t      len
) {
	if(ast_isnotZen(env)) {
		size_t index = marray_get_index(env->m.env, hash, cmp, leme, len);
		return index;
	}

	return ~SIZE_C(0);
}

Ast
lookup(
	Ast         env,
	uint64_t    hash,
	char const *leme,
	size_t      len,
	size_t      depth
) {
	if(ast_isnotZen(env)) do {
		Array  arr   = env->m.env;
		size_t index = marray_get_index(arr, hash, cmp, leme, len);
		if(~index) {
			return (index < marray_length(arr)) ? (
				marray_at(arr, Ast, index)
			) : (
				ZEN
			);
		}
	} while((--depth != 0) && ast_isnotZen(env = env->m.rexpr))
		;
	return ZEN;
}

size_t
define(
	Ast      env,
	uint64_t hash,
	Ast      def,
	Attr     attr
) {
	def->attr |= attr;

	if(ast_isnotZen(env)) {
		Array  arr   = env->m.env;
		size_t index = marray_length(arr);

		if(marray_push_back(arr, Ast, def)) {
			return marray_map_index(arr, hash, index);
		}
	}

	return ~SIZE_C(0);
}

//------------------------------------------------------------------------------

size_t
atenv(
	Ast env,
	Ast ident
) {
	size_t      n;
	uint64_t    hash = ident->m.hash;
	char const *cs   = StringToCharLiteral(ident->m.sval, &n);

	return locate(env, hash, cs, n);
}

Ast
inenv(
	Ast env,
	Ast ident
) {
	size_t      n;
	uint64_t    hash = ident->m.hash;
	char const *cs   = StringToCharLiteral(ident->m.sval, &n);

	return lookup(env, hash, cs, n, 0);
}

Ast
named_inenv(
	Ast         env,
	sloc_t      sloc,
	char const *name
) {
	String s = CharLiteralToString(name, strlen(name));
	assert(s != NULL);
	Ast    a = new_ast(sloc, AST_Identifier, s);

	return inenv(env, a);
}

Ast
addenv(
	Ast    env,
	sloc_t sloc,
	Ast    ident,
	Ast    def,
	Attr   attr
) {
	if(ast_isIdentifier(ident) || ast_isString(ident)) {
		def->attr = (def->attr & ~ATTR_NoAssign) | attr;

		size_t      n;
		uint64_t    hash = ident->m.hash;
		char const *cs   = StringToCharLiteral(ident->m.sval, &n);
		n                = locate(env, hash, cs, n);
		if(~n == 0) {
			def = new_ast(sloc, AST_Reference, ident->m.sval, def);
			n   = define(env, hash, def, attr);
			assert(~n != 0);
			return def;
		}

		ident = marray_at(env->m.env, Ast, n);
		assign(sloc, &ident->m.rexpr, def);
		return ident;
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
addenv_arg(
	Ast    to,
	Ast    env,
	sloc_t sloc,
	Ast    ident,
	Ast    arg
) {
	Attr attr = 0;

	if(ast_isTag(ident)) {
		if(ast_isZen(arg)) {
			arg = ident->m.rexpr;
		}
		ident = ident->m.lexpr;
		arg   = refeval(env, arg);
		arg   = dup_ast(sloc, arg);

	} else {
		arg  = subeval(env, arg);
		attr = arg->attr;
	}

	return addenv(to, sloc, ident, arg, attr);
}

Ast
addenv_args(
	Ast    to,
	Ast    env,
	sloc_t sloc,
	Ast    idents,
	Ast    args
) {
	for(;
		ast_isSequence(idents) && ast_isSequence(args);
		idents = idents->m.rexpr, args = args->m.rexpr
	) {
		addenv_arg(to, env, sloc, idents->m.lexpr, args->m.lexpr);
	}

	if(ast_isSequence(idents)) {
		addenv_arg(to, env, sloc, idents->m.lexpr, args);

		for(idents = idents->m.rexpr;
			ast_isSequence(idents);
			idents = idents->m.rexpr
		) {
			addenv_arg(to, env, sloc, idents->m.lexpr, ZEN);
		}
		addenv_arg(to, env, sloc, idents, ZEN);

	} else if(ast_isSequence(args)) {
		addenv_arg(to, env, sloc, idents, args->m.lexpr);

	} else {
		addenv_arg(to, env, sloc, idents, args);
	}

	return env;
}

Ast
addenv_operands(
	Ast    to,
	Ast    env,
	sloc_t sloc,
	Ast    idents,
	Ast    lexpr,
	Ast    rexpr
) {
	lexpr = subeval(env, lexpr);
	rexpr = subeval(env, rexpr);

	if(ast_isSequence(idents)) {
		addenv(to, sloc, idents->m.lexpr, lexpr, 0);

		idents = idents->m.rexpr;
		if(ast_isSequence(idents)) {
			addenv(to, sloc, idents->m.lexpr, rexpr, 0);

			for(idents = idents->m.rexpr;
				ast_isSequence(idents);
				idents = idents->m.rexpr
			) {
				addenv(to, sloc, idents->m.lexpr, ZEN, 0);
			}
			addenv(to, sloc, idents, ZEN, 0);

		} else {
			addenv(to, sloc, idents, rexpr, 0);
		}

	} else {
		addenv(to, sloc, idents, ast_isnotZen(lexpr) ? lexpr : rexpr, 0);
	}

	return env;
}

void
addenv_argv(
	Ast    to,
	sloc_t sloc,
	int    argc,
	char  *argv[]
) {
	Ast d = new_env(sloc, NULL);
	addenv_named(to, sloc, "argv", d, ATTR_NoAssign);

	for(int argi = 0; argi < argc; ++argi) {
		String s = CharLiteralToString(argv[argi], strlen(argv[argi]));
		assert(s != NULL);
		Ast    a = new_ast(sloc, AST_String, s);
		a->attr |= ATTR_NoAssign;
		bool   appended = marray_push_back(d->m.env, Ast, a);
		assert(appended);
	}

	d = new_ast(sloc, AST_Integer, (uint64_t)argc);
	addenv_named(to, sloc, "argc", d, ATTR_NoAssign);
}

void
addenv_named(
	Ast         to,
	sloc_t      sloc,
	char const *name,
	Ast         ast,
	Attr        attr
) {
	String s = CharLiteralToString(name, strlen(name));
	assert(s != NULL);
	Ast    a = new_ast(sloc, AST_Identifier, s);

	addenv(to, 0, a, ast, attr);
}

//------------------------------------------------------------------------------

Ast operators = NULL;
Ast globals   = NULL;
Ast statics   = NULL;
Ast locals    = NULL;

int
initialise_env(
	void
) {
	static bool initialise = true;

	if(initialise) {
		initialise = false;

		operators = new_env(0, NULL);
		globals   = new_env(0, NULL);
		Ast env   = source_env(0);
		statics   = new_ast(0, AST_Environment, env->m.env, globals);
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

Ast source_env(
	unsigned source
) {
	static Ast source_environments =  NULL;

	if(!source_environments) {
		source_environments = new_env(0, NULL);

		bool appended = marray_push_back(globals->m.env, Ast, source_environments);
		assert(appended);
	}

	assert(source <= marray_length(source_environments->m.env));
	if(source == marray_length(source_environments->m.env)) {

		Ast  sourcenv = new_env(make_sloc(source, 0, 0, 0), globals);
		bool appended = marray_push_back(source_environments->m.env, Ast, sourcenv);
		assert(appended);
	}

	return marray_at(source_environments->m.env, Ast, source);
}

//------------------------------------------------------------------------------

Ast
getopr(
	size_t index
) {
	return (index < marray_length(operators->m.env)) ? (
		marray_at(operators->m.env, Ast, index)
	) : (
		ZEN
	);
}

char const *
getops(
	size_t index
) {
	Ast ast = getopr(index);
	return ast_isnotZen(ast) ? (
		StringToCharLiteral(ast->m.sval, NULL)
	) : (
		"(+)"
	);
}

