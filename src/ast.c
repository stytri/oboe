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
#include "assert.h"
#include "utf8.h"
#include "parse.h"
#include "lex.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>

//------------------------------------------------------------------------------

Ast ZEN = NULL;

//------------------------------------------------------------------------------

static inline String
dupstr(
	char const *cs,
	size_t      n
) {
	String t = CharLiteralToString(cs, n);
	assert(t != NULL);

	return t;
}

static uint64_t
makint(
	char const *cs,
	size_t      n
) {
	uint64_t val = 0;

	char tmp[sizeof(val) * CHAR_BIT];
	char *buf = (n >= sizeof(tmp)) ? (
		malloc(n+1)
	) : (
		tmp
	);
	assert(buf != NULL);
	memcpy(buf, cs, n);
	buf[n] = '\0';

	val = strtou(buf, NULL);

	if(buf != tmp) {
		free(buf);
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
		malloc(n+1)
	) : (
		tmp
	);
	assert(buf != NULL);
	memcpy(buf, cs, n);
	buf[n] = '\0';

	val = strtod(buf, NULL);

	if(buf != tmp) {
		free(buf);
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

	return t;
}

static unsigned
makopr(
	char const *cs,
	size_t      n
) {
	uint64_t hash  = memhash(cs, n, 0);
	size_t   index = locate(operators, hash, cs, n);

	Ast ast = getopr(index);
	if(ast_isOperatorAlias(ast)) {
		cs    = StringToCharLiteral(ast->m.tval, &n);
		hash  = memhash(cs, n, 0);
		index = locate(operators, hash, cs, n);
	}

	return index;
}

//------------------------------------------------------------------------------

#define MIN_GC_THRESHOLD  ((CHAR_BIT * sizeof(size_t)) * 1024)
#define MAX_GC_THRESHOLD  BIT_ROUND(SIZE_MAX/2)

static size_t gc_threshold      =  MIN_GC_THRESHOLD;
static size_t low_gc_threshold  =  MIN_GC_THRESHOLD / 3;
static size_t high_gc_threshold = (MIN_GC_THRESHOLD / 3) * 2;

void
run_gc(
	void
) {
	gc_mark_and_sweep();

	if(gc_total_size() > high_gc_threshold) {
		if(gc_threshold < MAX_GC_THRESHOLD) {
			gc_threshold     *= 2;
			low_gc_threshold  = gc_threshold / 3;
			high_gc_threshold = low_gc_threshold * 2;
		}

	} else if(gc_total_size() < low_gc_threshold) {
		if(gc_threshold > MIN_GC_THRESHOLD) {
			gc_threshold     /= 2;
			low_gc_threshold  = gc_threshold / 3;
			high_gc_threshold = low_gc_threshold * 2;
		}
	}

	return;
}

//------------------------------------------------------------------------------

static void
ast_gc_mark(
	void const *p,
	void      (*gc_mark)(void const *)
) {
	Ast ast = (Ast)p;

	switch(ast->type) {
	default: {
#	define ENUM(Name)       } break; case AST_##Name: {
#	define MARK(...)          __VA_ARGS__;

#	include "oboe.enum"
	}}

	return;
}

static void
ast_gc_sweep(
	void const *p
) {
	Ast ast = (Ast)p;

	switch(ast->type) {
	default: {
#	define ENUM(Name)       } break; case AST_##Name: {
#	define SWEEP(...)         __VA_ARGS__;

#	include "oboe.enum"
	}}

	memset(ast, 0, sizeof(*ast));
	gc_free(ast);
	return;
}

static Ast
alloc_ast(
	void
) {
	if(gc_total_size() >= gc_threshold) {
		run_gc();
	}

	Ast ast = gc_malloc(sizeof(*ast), ast_gc_mark, ast_gc_sweep);
	assert(ast != NULL);
	*ast = (struct ast){ AST_Void, 0, 0, 0, {{ NULL }, { NULL }} };
	return ast;
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

void
initialise_ast(
	void
) {
	if(!ZEN) {
		ZEN = alloc_ast();
		assert(ZEN != NULL);
		ZEN->type = AST_Zen;
		ZEN->attr = ATTR_NoAssign;
		gc_push(ZEN);
	}
}

Ast
new_ast_from_lexeme(
	sloc_t      sloc,
	char const *leme,
	size_t      len,
	...
) {
	Ast ast = NULL;

	va_list va;
	va_start(va, len);

	Type     type;
	char32_t c ='\0';

	char const *cs;
	c = utf8chr(leme, &cs);

	if(is_Digit(c)) {
		type = is_float(leme, len) ? AST_Float : AST_Integer;

	} else if(is_ID_Start(c)) {
		type = AST_Identifier;

	} else switch(c) {
	case '\0': {
			va_list vc;
			va_copy(vc, va);
			Ast lexpr = va_arg(vc, Ast);
			Ast rexpr = va_arg(vc, Ast);
			va_end(vc);
			if(ast_isArray(rexpr)) {
				if(ast_isZen(rexpr->m.lexpr)) {
					rexpr->m.lexpr = lexpr;
					va_end(va);
					return rexpr;
				}
			}
			if(ast_isOperator(lexpr)) {
				if(ast_isZen(lexpr->m.rexpr)) {
					lexpr->m.rexpr = rexpr;
					va_end(va);
					return lexpr;
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
		va_end(va);
		return ZEN;
	}

	ast = alloc_ast();

	ast->type = type;
	ast->sloc = sloc;

	switch(type) {
	default: {
#	define ENUM(Name)       } break; case AST_##Name: {
#	define NEW(...)         __VA_ARGS__;

#	define INTEGER(...)     (ast->attr |= ATTR_CopyOnAssign, makint(leme, len)   )
#	define FLOAT(...)       (ast->attr |= ATTR_CopyOnAssign, makdbl(leme, len)   )
#	define CHARACTER(...)   (ast->attr |= ATTR_CopyOnAssign, makchr(leme, len)   )
#	define STRING(...)      (ast->attr |= ATTR_CopyOnAssign, makstr(leme, len, c))
#	define IDENTIFIER(...)  (                                dupstr(leme, len)   )
#	define OPERATOR(...)    (                                makopr(leme, len)   )

#	include "oboe.enum"

#	undef INTEGER
#	undef FLOAT
#	undef CHARACTER
#	undef STRING
#	undef IDENTIFIER
#	undef OPERATOR
	}}

	va_end(va);

	return gc_push(ast);
}

Ast
new_ast(
	sloc_t      sloc,
	Type        type,
	...
) {
	Ast ast = NULL;

	if(type == AST_Zen) {
		return ZEN;
	}

	ast = alloc_ast();

	ast->type = type;
	ast->sloc = sloc;

	va_list va;
	va_start(va, type);

	switch(type) {
	default: {
#	define ENUM(Name)       } break; case AST_##Name: {
#	define NEW(...)         __VA_ARGS__;

#	define INTEGER(...)     va_arg(va, uint64_t)
#	define FLOAT(...)       va_arg(va, double)
#	define CHARACTER(...)   va_arg(va, int)
#	define STRING(...)      va_arg(va, String)
#	define IDENTIFIER(...)  va_arg(va, String)
#	define OPERATOR(...)    va_arg(va, unsigned)

#	include "oboe.enum"

#	undef INTEGER
#	undef FLOAT
#	undef CHARACTER
#	undef STRING
#	undef IDENTIFIER
#	undef OPERATOR
	}}

	va_end(va);

	return gc_push(ast);
}

Ast
dup_ast(
	sloc_t sloc,
	Ast    ast
) {
	if(ast_isnotZen(ast)) {
		Ast dup = alloc_ast();

		memcpy(dup, ast, sizeof(*ast));

		if(ast_isRemoveCopyOnAssign(dup)) {
			dup->attr &= ~ATTR_CopyOnAssign;
		}
		dup->attr &= ~ATTR_NoAssign;
		dup->sloc = sloc;

		switch(ast->type) {
		default: {
#		define ENUM(Name)       } break; case AST_##Name: {
#		define DUP(...)         __VA_ARGS__;

#		include "oboe.enum"
		}}

		return gc_push(dup);
	}

	return ast;
}

Ast
dup_ref(
	sloc_t sloc,
	Ast    ast
) {
	if(ast_isReference(ast)) {
		Ast dup = alloc_ast();

		memcpy(dup, ast, sizeof(*ast));

		return gc_push(dup);
	}

	return dup_ast(sloc, ast);
}

