#ifndef AST_H_INCLUDED
#define AST_H_INCLUDED
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

#include "stdtypes.h"
#include "string.h"
#include "array.h"
#include "bitmac.h"
#include "sloc.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef struct ast *Ast;

typedef enum {
#	define ENUM(Name)  AST_##Name,
#	include "oboe.enum"
	N_AST_Types
} Type;

enum { TYPE_MASK = BIT_MASK(N_AST_Types) };
enum { TYPE_BIT  = BIT_COUNT(TYPE_MASK) };
enum { QUAL_BIT  = (CHAR_BIT * sizeof(unsigned)) - TYPE_BIT };

typedef enum {
#	define ENUM(Name)  ERR_##Name,
#	include "oboerr.enum"
	N_Errors
} Error;

//------------------------------------------------------------------------------

typedef Ast (*BuiltinFn)(
	Ast    env,
	sloc_t sloc,
	Ast    arg
);

typedef Ast (*BuiltinOp)(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
);

struct ast {
	Type      type : TYPE_BIT;
	unsigned  qual : QUAL_BIT;
	sloc_t    sloc;
	struct {
		union {
		void     *lptr;
		Ast       lexpr;
		uint64_t  ival;
		double    fval;
		String    sval;
		Array     env;
		};
		union {
		void     *rptr;
		Ast       rexpr;
		uint64_t  hash;
		BuiltinFn bfn;
		BuiltinOp bop;
		};
	}	m;
};

extern struct ast *const ZEN;

//------------------------------------------------------------------------------

static inline Type
ast_type(
	Ast ast
) {
	return ast ? ast->type : AST_Zen;
}

extern char const *
ast_typename(
	Ast ast
);

#define ENUM(Name) \
static inline bool \
ast_is##Name( \
	Ast ast \
) { \
	return ast && (ast->type == AST_##Name); \
}
#include "oboe.enum"

static inline bool
ast_isNumeric(
	Ast ast
) {
	return ast && ((ast->type == AST_Integer) || (ast->type == AST_Float));
}

static inline bool
ast_isDeferred(
	Ast ast
) {
	return ast && ((ast->type == AST_Reference) || (ast->type == AST_Quoted));
}

#define ENUM(Name) \
static inline bool \
ast_isnot##Name( \
	Ast ast \
) { \
	return ast && (ast->type != AST_##Name); \
}
#include "oboe.enum"

static inline bool
ast_isnotNumeric(
	Ast ast
) {
	return ast && ((ast->type != AST_Integer) && (ast->type != AST_Float));
}

static inline bool
ast_isnotDeferred(
	Ast ast
) {
	return ast && ((ast->type != AST_Reference) && (ast->type != AST_Quoted));
}

static inline bool
ast_isOp(
	Ast      ast,
	unsigned qual
) {
	return ast_isOperator(ast) && (ast->qual == qual);
}

static inline bool
ast_isType(
	Ast      ast,
	unsigned qual
) {
	return ast_isOpaqueDataType(ast) && (ast->qual == qual);
}

static inline bool
ast_isReferenceType(
	Ast      ast,
	unsigned qual
) {
	return ast_isOpaqueDataReference(ast) && (ast->qual == qual);
}

//------------------------------------------------------------------------------

extern bool
ast_isTag(
	Ast ast
);

extern bool
ast_isApplicate(
	Ast ast
);

extern bool
ast_isArray(
	Ast ast
);

extern bool
ast_toBool(
	Ast ast
);

//------------------------------------------------------------------------------

extern Ast
new_ast(
	sloc_t      sloc,
	char const *leme,
	size_t      len,
	...
);

extern Ast
dup_ast(
	sloc_t sloc,
	Ast    ast
);

extern void
run_gc(
	void
);

//------------------------------------------------------------------------------

extern Ast
oboerr(
	sloc_t sloc,
	Error  err
);

extern char const *
oboerrs(
	Error err
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef AST_H_INCLUDED
