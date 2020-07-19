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
#include "marray.h"
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

typedef enum {
	ATTR_NoEvaluate         = 0x01,
	ATTR_NoAssign           = 0x02,
	ATTR_CopyOnAssign       = 0x04,
	ATTR_RetainCopyOnAssign = 0x08,
	ATTR_BIT                = 4
} Attr;

enum { TYPE_MASK = BIT_MASK(N_AST_Types) };
enum { TYPE_BIT  = BIT_COUNT(TYPE_MASK) };
enum { QUAL_BIT  = (CHAR_BIT * sizeof(unsigned)) - TYPE_BIT - ATTR_BIT };

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
	unsigned  attr : ATTR_BIT;
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
		String    tval;
		uint64_t  hash;
		BuiltinFn bfn;
		BuiltinOp bop;
		};
	}	m;
};

extern struct ast *ZEN;

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
	return ast
		&& ((ast->type == AST_Boolean) || (ast->type == AST_Integer) || (ast->type == AST_Float));
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
	return ast
		&& ((ast->type != AST_Boolean) && (ast->type != AST_Integer) && (ast->type != AST_Float));
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
ast_isnotOp(
	Ast      ast,
	unsigned qual
) {
	return ast_isnotOperator(ast) || (ast->qual != qual);
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

static inline bool
ast_isAssignable(
	Ast ast
) {
	return ast && !(ast->attr & ATTR_NoAssign);
}

static inline bool
ast_isCopyOnAssign(
	Ast ast
) {
	return ast && (ast->attr & ATTR_CopyOnAssign);
}

static inline bool
ast_isRetainCopyOnAssign(
	Ast ast
) {
	return ast && (ast->attr & ATTR_RetainCopyOnAssign);
}

static inline bool
ast_isRemoveCopyOnAssign(
	Ast ast
) {
	return ast && (ast->attr & ATTR_CopyOnAssign) && !(ast->attr & ATTR_RetainCopyOnAssign);
}

//------------------------------------------------------------------------------

extern bool
ast_isTag(
	Ast ast
);

extern bool
ast_isTagRef(
	Ast ast
);

extern bool
ast_isConst(
	Ast ast
);

extern bool
ast_isGlobalTag(
	Ast ast
);

extern bool
ast_isGlobalTagRef(
	Ast ast
);

extern bool
ast_isGlobalConst(
	Ast ast
);

extern bool
ast_isDeclaration(
	Ast ast
);

extern bool
ast_isApplicate(
	Ast ast
);

extern bool
ast_isAssign(
	Ast ast
);

extern bool
ast_isnotAssign(
	Ast ast
);

extern bool
ast_isBlock(
	Ast ast
);

extern bool
ast_isnotBlock(
	Ast ast
);

extern bool
ast_isArray(
	Ast ast
);

extern bool
ast_isnotArray(
	Ast ast
);

extern bool
ast_isRange(
	Ast ast
);

extern bool
ast_isnotRange(
	Ast ast
);

extern bool
ast_isRelational(
	Ast ast
);

extern bool
ast_isBracketed(
	Ast ast
);

extern bool
ast_isnotBracketed(
	Ast ast
);

extern uint64_t
ast_toInteger(
	Ast ast
);

extern double
ast_toFloat(
	Ast ast
);

extern bool
ast_toBool(
	Ast ast
);

//------------------------------------------------------------------------------

extern void
initialise_ast(
	void
);

extern Ast
new_ast_from_lexeme(
	sloc_t      sloc,
	char const *leme,
	size_t      len,
	...
);

extern Ast
new_ast(
	sloc_t      sloc,
	Type        type,
	...
);

extern Ast
dup_ast(
	sloc_t sloc,
	Ast    ast
);

extern Ast
dup_ref(
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
