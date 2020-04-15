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
#include "marray.h"
#include "assert.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>

//------------------------------------------------------------------------------

struct env_stats env_stats;

//------------------------------------------------------------------------------

typedef XMEM_STRUCT(struct array, env) xmem_env;
#define XMEM_ENV(...)  (xmem_env) { \
	{ sizeof(xmem_env), 0 }, \
	{ 0, 0, 0, (uintptr_t)NULL } \
}
static struct array env_pool      = ARRAY();
static Array        env_free_list = NULL;

//------------------------------------------------------------------------------

Ast
new_env(
	sloc_t sloc,
	Ast    outer
) {
	Array env;

	if(env_free_list != NULL) {
		env           = env_free_list;
		env_free_list = (Array)env->map;
		*env = ARRAY();

	} else {
		xmem_env *xenv = marray_create_back(&env_pool, xmem_env);
		assert(xenv != NULL);
		*xenv = XMEM_ENV();
		env = &xenv->data;
	}

	*xmemheader(env) = 1;

	Ast ast = new_ast(sloc, NULL, AST_Environment, env, outer);

	env_stats.total++;
	env_stats.live++;

	return ast;
}

Ast
dup_env(
	sloc_t sloc,
	Ast    ast,
	Ast    outer
) {
	Ast dup = dup_ast(sloc, ast);
	if(outer != ast) {
		dup->m.rexpr = outer;
	}

	++*xmemheader(dup->m.env);

	return dup;
}

void
del_env(
	Ast ast
) {
	Array env = ast->m.env;

	if(env && ((--*xmemheader(env)) == 0)) {
		array_free(env);

		memset(env, 0, sizeof(*env));
		env->map      = (uintptr_t)env_free_list;
		env_free_list = env;

		env_stats.dead++;
		env_stats.live--;
	}

	return;
}

//------------------------------------------------------------------------------

static int
cmp(
	Array       arr,
	size_t      index,
	void const *key,
	size_t      n
) {
	Ast ast = array_at(arr, Ast, index);
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
		size_t index = array_get_index(env->m.env, hash, cmp, leme, len);
		return index;
	}

	return ~(size_t)0;
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
		size_t index = array_get_index(arr, hash, cmp, leme, len);
		if(~index) {
			return (index < array_length(arr)) ? (
				array_at(arr, Ast, index)
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
	Ast      def
) {
	if(ast_isnotZen(env)) {
		Array  arr   = env->m.env;
		size_t index = array_length(arr);

		if(array_push_back(arr, Ast, def)) {
			return array_map_index(arr, hash, index);
		}
	}

	return ~(size_t)0;
}

//------------------------------------------------------------------------------

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
	Ast    a = new_ast(sloc, NULL, AST_Identifier, s);

	return inenv(env, a);
}

Ast
addenv(
	Ast    env,
	sloc_t sloc,
	Ast    ident,
	Ast    def
) {
	if(ast_isIdentifier(ident) || ast_isString(ident)) {
		size_t      n;
		uint64_t    hash = ident->m.hash;
		char const *cs   = StringToCharLiteral(ident->m.sval, &n);
		n                = locate(env, hash, cs, n);
		if(~n == 0) {
			def = new_ast(sloc, NULL, AST_Reference, ident->m.sval, def);
			n   = define(env, hash, def);
			assert(~n != 0);
			return def;
		}
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
	if(ast_isTag(ident)) {
		if(ast_isZen(arg)) {
			arg = ident->m.rexpr;
		}
		ident = ident->m.lexpr;
		arg   = refeval(env, arg);
		arg   = dup_ast(sloc, arg);

	} else {
		arg = subeval(env, arg);
	}

	return addenv(to, sloc, ident, arg);
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
	if(ast_isIdentifier(lexpr)) {
		lexpr = subeval(env, lexpr);
	}
	if(ast_isIdentifier(rexpr)) {
		rexpr = subeval(env, rexpr);
	}

	if(ast_isSequence(idents)) {
		addenv(to, sloc, idents->m.lexpr, lexpr);

		idents = idents->m.rexpr;
		if(ast_isSequence(idents)) {
			addenv(to, sloc, idents->m.lexpr, rexpr);

			for(idents = idents->m.rexpr;
				ast_isSequence(idents);
				idents = idents->m.rexpr
			) {
				addenv(to, sloc, idents->m.lexpr, ZEN);
			}
			addenv(to, sloc, idents, ZEN);

		} else {
			addenv(to, sloc, idents, rexpr);
		}

	} else {
		addenv(to, sloc, idents, ast_isnotZen(lexpr) ? lexpr : rexpr);
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
	addenv_named(to, sloc, "argv", d);

	for(int argi = 0; argi < argc; ++argi) {
		String s = CharLiteralToString(argv[argi], strlen(argv[argi]));
		assert(s != NULL);
		Ast    a = new_ast(sloc, NULL, AST_String, s);
		bool   appended = array_push_back(d->m.env, Ast, a);
		assert(appended);
	}

	d = new_ast(sloc, NULL, AST_Integer, (uint64_t)argc);
	addenv_named(to, sloc, "argc", d);
}

void
addenv_named(
	Ast         to,
	sloc_t      sloc,
	char const *name,
	Ast         ast
) {
	String s = CharLiteralToString(name, strlen(name));
	assert(s != NULL);
	Ast    a = new_ast(sloc, NULL, AST_Identifier, s);

	addenv(to, 0, a, ast);
}

//------------------------------------------------------------------------------

Ast operators = NULL;
Ast globals   = NULL;

int
initialise_env(
	void
) {
	operators = new_env(0, NULL);
	globals   = new_env(0, NULL);

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

Ast
getopr(
	size_t index
) {
	return (index < array_length(operators->m.env)) ? (
		array_at(operators->m.env, Ast, index)
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

