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
#include "odt.h"
#include "hash.h"
#include "strlib.h"
#include "marray.h"
#include "assert.h"
#include "utf8.h"
#include "parse.h"
#include "lex.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>

//------------------------------------------------------------------------------

XMEM(struct ast, ZEN, (struct ast){ AST_Zen, 0, 0, {{ NULL }, { NULL }} });

//------------------------------------------------------------------------------

static void *
add_leaf_to_gc(
	void const *p
);

static void *
add_branch_to_gc(
	void const *p
);

static void
ast_gc_mark(
	void const *p
);

static inline String
dupstr(
	char const *cs,
	size_t      n
) {
	String t = CharLiteralToString(cs, n);
	assert(t != NULL);

	return add_leaf_to_gc(t);
}

static uint64_t
makint(
	char const *cs,
	size_t      n
) {
	uint64_t val = 0;

	char tmp[sizeof(val) * CHAR_BIT];
	char *buf = (n >= sizeof(tmp)) ? (
		xmalloc(n+1)
	) : (
		tmp
	);
	assert(buf != NULL);
	memcpy(buf, cs, n);
	buf[n] = '\0';

	val = strtou(buf, NULL);

	if(buf != tmp) {
		xfree(buf);
	}

	return val;
}

static double
makdbl(
	char const *cs,
	size_t      n
) {
	double val = 0.0;

	char tmp[sizeof(val) * CHAR_BIT];
	char *buf = (n >= sizeof(tmp)) ? (
		xmalloc(n+1)
	) : (
		tmp
	);
	assert(buf != NULL);
	memcpy(buf, cs, n);
	buf[n] = '\0';

	val = strtod(buf, NULL);

	if(buf != tmp) {
		xfree(buf);
	}

	return val;
}

static int
makchr(
	char const *cs,
	size_t      n
) {
	if(n < 3) {
		return 0;
	}

	return strtoc(cs+1, NULL);
}

static String
makstr(
	char const *cs,
	size_t      n,
	int         q
) {
	++cs;
	--n;

	if(q == '"') {
		if((n > 0) && (cs[n-1] == '"')) {
			--n;
		}
		return dupstr(cs, n);
	}
	String t = UnEscapeString(NULL, cs, NULL, q);
	assert(t != NULL);

	return add_leaf_to_gc(t);
}

static unsigned
makopr(
	char const *cs,
	size_t      n
) {
	uint64_t hash  = memhash(cs, n, 0);
	size_t   index = locate(operators, hash, cs, n);

	return index;
}

static inline void
mrkptr(
	void const *p
) {
	if(p) {
		gc_mark(ast_gc_mark, p);
	}
}

static void
mrkenv(
	Array env
) {
	if(env) {
		for(size_t i = array_length(env); i-- > 0;) {
			void const *p = array_at(env, void const *, i);
			if(p) {
				gc_mark(ast_gc_mark, p);
			}
		}
	}

	return;
}

//------------------------------------------------------------------------------

#define MIN_GC_THRESHOLD  (CHAR_BIT * sizeof(size_t))
#define MAX_GC_THRESHOLD  (SIZE_MAX / MIN_GC_THRESHOLD)

static size_t gc_threshold      =  MIN_GC_THRESHOLD;
static size_t low_gc_threshold  =  MIN_GC_THRESHOLD / 3;
static size_t high_gc_threshold = (MIN_GC_THRESHOLD / 3) * 2;

typedef XMEM_STRUCT(struct ast, ast) xmem_ast;
#define XMEM_AST(...)  (xmem_ast) { \
	{ sizeof(xmem_ast), 0 }, \
	{ AST_Zen, 0, 0, {{ NULL }, { NULL }} } \
}
static struct array ast_pool      = ARRAY();
static Ast          ast_free_list = NULL;

//------------------------------------------------------------------------------

static inline void *
add_leaf_to_gc(
	void const *p
) {
	return gc_leaf(p);
}

static inline void *
add_branch_to_gc(
	void const *p
) {
	return gc_push(gc_branch(p));
}

static inline void *
remove_from_gc(
	void const *p
) {
	return gc_remove(p);
}

static void
ast_gc_mark(
	void const *p
) {
	Ast ast = (Ast)p;

	switch(ast->type) {
	default: {
#	define ENUM(Name)      } break; case AST_##Name: {
#	define GC(...)         __VA_ARGS__;

#	define MARK(P)         mrkptr(P)
#	define ENVIRONMENT(E)  mrkenv((E)->m.env)

#	include "oboe.enum"

#	undef MARK
#	undef ENVIRONMENT
	}}

	return;
}

static void
ast_gc_sweep(
	void const *p
) {
	if(gc_is_leaf(p)) {
		xfree(remove_from_gc(p));
		return;
	}

	Ast ast = remove_from_gc(p);

	switch(ast->type) {
	default: {
#	define ENUM(Name)      } break; case AST_##Name: {
#	define DEL(...)        __VA_ARGS__;

#	define ENVIRONMENT(E)  del_env(E)

#	include "oboe.enum"

#	undef ENVIRONMENT
	}}

	memset(ast, 0, sizeof(*ast));
	ast->m.lexpr    = ast_free_list;
	ast_free_list = ast;

	return;
}

void
run_gc(
	void
) {
	gc_stats.collect++;

	gc(ast_gc_mark, ast_gc_sweep);

	if(gc_stats.live > high_gc_threshold) {
		if(gc_threshold < MAX_GC_THRESHOLD) {
			gc_threshold     *= 2;
			low_gc_threshold  = gc_threshold / 3;
			high_gc_threshold = low_gc_threshold * 2;
		}

	} else if(gc_stats.live < low_gc_threshold) {
		if(gc_threshold > MIN_GC_THRESHOLD) {
			gc_threshold     /= 2;
			low_gc_threshold  = gc_threshold / 3;
			high_gc_threshold = low_gc_threshold * 2;
		}
	}

	return;
}

static Ast
alloc_ast(
	void
) {
	if((ast_free_list == NULL)
		&& (gc_stats.live >= gc_threshold)
	) {
		run_gc();
	}

	if(ast_free_list != NULL) {
		Ast ast       = ast_free_list;
		ast_free_list = ast->m.lexpr;
		return ast;
	}

	xmem_ast *xast = marray_create_back(&ast_pool, xmem_ast);
	assert(xast != NULL);
	*xast = XMEM_AST();
	return &xast->data;
}

//------------------------------------------------------------------------------

char const *
ast_typename(
	Ast ast
) {
	static char const *typename[] = {
#	define ENUM(Name,...) #Name,
#	include "oboe.enum"
	};
	enum { N_TypeNames = sizeof(typename) / sizeof(typename[0]) };

	static_assert((size_t)N_AST_Types == (size_t)N_TypeNames, "N_AST_Types == N_TypeNames");

	if(ast->type < N_AST_Types) {
		return typename[ast->type];
	}

	return "";
}

//------------------------------------------------------------------------------

Ast
new_ast(
	sloc_t      sloc,
	char const *leme,
	size_t      n,
	...
) {
	Ast ast = NULL;

	va_list va;
	va_start(va, n);

	Type     type;
	char32_t c ='\0';

	if(leme) {
		char const *cs;
		c = utf8chr(leme, &cs);

		if(is_Digit(c)) {
			type = is_float(leme, n) ? AST_Float : AST_Integer;

		} else if(is_ID_Start(c)) {
			type = AST_Identifier;

		} else switch(c) {
		case '\0': {
				va_list vc;
				va_copy(vc, va);
				Ast lexpr = va_arg(vc, Ast);
				Ast rexpr = va_arg(vc, Ast);
				va_end(vc);
				if(ast_isOperator(lexpr)) {
					if(ast_isZen(lexpr->m.rexpr)) {
						lexpr->m.rexpr = rexpr;
						va_end(va);
						return lexpr;
					}
				} else if(ast_isArray(rexpr)) {
					if(ast_isZen(rexpr->m.lexpr)) {
						rexpr->m.lexpr = lexpr;
						va_end(va);
						return rexpr;
					}
				}
			}
			nobreak;
		case '[' :
		case '{' :
		default  : type = AST_Operator;   break;
		case ',' : type = AST_Sequence;   break;
		case ';' : type = AST_Assemblage; break;
		case '"' : type = AST_String;     break;
		case '\'': type = AST_String;     break;
		case '`' : type = AST_Character;  break;
		case '(' :
			return ZEN;
		}

	} else {
		if(n == AST_Zen) {
			va_end(va);
			return ZEN;
		}
		type = n;
	}

	ast = alloc_ast();

	ast->type = type;
	ast->sloc = sloc;

	switch(type) {
	default: {
#	define ENUM(Name)      } break; case AST_##Name: {
#	define NEW(...)        __VA_ARGS__;

#	define INTEGER(S,N)    (S) ? makint((S), (N))    : va_arg(va, uint64_t)
#	define FLOAT(S,N)      (S) ? makdbl((S), (N))    : va_arg(va, double)
#	define CHARACTER(S,N)  (S) ? makchr((S), (N))    : va_arg(va, int)
#	define STRING(S,N)     (S) ? makstr((S), (N), c) : add_leaf_to_gc(va_arg(va, String))
#	define IDENTIFIER(S,N) (S) ? dupstr((S), (N))    : add_leaf_to_gc(va_arg(va, String))
#	define OPERATOROF(S,N) (S) ? makopr((S), (N))    : va_arg(va, unsigned)

#	include "oboe.enum"

#	undef INTEGER
#	undef FLOAT
#	undef CHARACTER
#	undef STRING
#	undef IDENTIFIER
#	undef OPERATOROF
	}}

	va_end(va);

	return add_branch_to_gc(ast);
}

Ast
dup_ast(
	sloc_t sloc,
	Ast    ast
) {
	if(ast_isnotZen(ast)) {
		Ast dup = alloc_ast();

		memcpy(dup, ast, sizeof(*ast));
		dup->sloc = sloc;

		return add_branch_to_gc(dup);
	}

	return ast;
}

