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
#include "builtins.h"
#include "searchpaths.h"
#include "mapfile.h"
#include "strlib.h"
#include "assert.h"
#include "parse.h"
#include "tostr.h"
#include "hash.h"
#include "utf8.h"
#include "eval.h"
#include "env.h"
#include "odt.h"
#include "gc.h"
#include <stdlib.h>
#include <math.h>

//------------------------------------------------------------------------------

static inline size_t
minz(
	size_t const a,
	size_t const b
) {
	return a < b ? a : b;
}

static inline size_t
maxz(
	size_t const a,
	size_t const b
) {
	return a > b ? a : b;
}

//------------------------------------------------------------------------------

typedef enum {
	BY_Value,
	BY_Ref
} By;

//------------------------------------------------------------------------------

#define DOUBLE_BUILTINS \
	DOUBLE_MATH1_FN(ceil) \
	DOUBLE_MATH1_FN(floor) \
	DOUBLE_MATH1_FN(trunc) \
	DOUBLE_MATH1_FN(round) \
	DOUBLE_MATH1_FN(cos) \
	DOUBLE_MATH1_FN(sin) \
	DOUBLE_MATH1_FN(tan) \
	DOUBLE_MATH1_FN(cosh) \
	DOUBLE_MATH1_FN(sinh) \
	DOUBLE_MATH1_FN(tanh) \
	DOUBLE_MATH1_FN(acos) \
	DOUBLE_MATH1_FN(asin) \
	DOUBLE_MATH1_FN(atan) \
	DOUBLE_MATH2_FN(atan2) \
	DOUBLE_MATH1_FN(acosh) \
	DOUBLE_MATH1_FN(asinh) \
	DOUBLE_MATH1_FN(atanh) \
	DOUBLE_MATH1_FN(exp) \
	DOUBLE_MATH1_FN(exp2) \
	DOUBLE_MATH1_FN(log) \
	DOUBLE_MATH1_FN(log2) \
	DOUBLE_MATH1_FN(log10) \
	DOUBLE_MATH1_FN(sqrt) \
	DOUBLE_MATH1_FN(cbrt) \
	DOUBLE_MATH2_FN(fmin) \
	DOUBLE_MATH2_FN(fmax) \
	DOUBLE_MATH2_FN(hypot) \
	DOUBLE_MATH2_FN(pow)

//------------------------------------------------------------------------------

// Operators

static unsigned builtin_applicate_enum      = -1;
static unsigned builtin_tag_enum            = -1;
static unsigned builtin_tag_ref_enum        = -1;
static unsigned builtin_const_enum          = -1;
static unsigned builtin_global_tag_enum     = -1;
static unsigned builtin_global_tag_ref_enum = -1;
static unsigned builtin_global_const_enum   = -1;
static unsigned builtin_assign_enum         = -1;
static unsigned builtin_assign_ref_enum     = -1;
static unsigned builtin_assign_land_enum    = -1;
static unsigned builtin_assign_lor_enum     = -1;
static unsigned builtin_assign_and_enum     = -1;
static unsigned builtin_assign_or_enum      = -1;
static unsigned builtin_assign_xor_enum     = -1;
static unsigned builtin_assign_add_enum     = -1;
static unsigned builtin_assign_sub_enum     = -1;
static unsigned builtin_assign_mul_enum     = -1;
static unsigned builtin_assign_div_enum     = -1;
static unsigned builtin_assign_mod_enum     = -1;
static unsigned builtin_assign_shl_enum     = -1;
static unsigned builtin_assign_shr_enum     = -1;
static unsigned builtin_assign_exl_enum     = -1;
static unsigned builtin_assign_exr_enum     = -1;
static unsigned builtin_assign_rol_enum     = -1;
static unsigned builtin_assign_ror_enum     = -1;
static unsigned builtin_exchange_enum       = -1;
static unsigned builtin_if_enum             = -1;
static unsigned builtin_ifnot_enum          = -1;
static unsigned builtin_case_enum           = -1;
static unsigned builtin_while_enum          = -1;
static unsigned builtin_until_enum          = -1;
static unsigned builtin_land_enum           = -1;
static unsigned builtin_lor_enum            = -1;
static unsigned builtin_lt_enum             = -1;
static unsigned builtin_lte_enum            = -1;
static unsigned builtin_eq_enum             = -1;
static unsigned builtin_neq_enum            = -1;
static unsigned builtin_gte_enum            = -1;
static unsigned builtin_gt_enum             = -1;
static unsigned builtin_and_enum            = -1;
static unsigned builtin_or_enum             = -1;
static unsigned builtin_xor_enum            = -1;
static unsigned builtin_add_enum            = -1;
static unsigned builtin_sub_enum            = -1;
static unsigned builtin_mul_enum            = -1;
static unsigned builtin_div_enum            = -1;
static unsigned builtin_mod_enum            = -1;
static unsigned builtin_shl_enum            = -1;
static unsigned builtin_shr_enum            = -1;
static unsigned builtin_exl_enum            = -1;
static unsigned builtin_exr_enum            = -1;
static unsigned builtin_rol_enum            = -1;
static unsigned builtin_ror_enum            = -1;
static unsigned builtin_array_enum          = -1;
static unsigned builtin_range_enum          = -1;

// Intrinsic Functions

#undef  DOUBLE_MATH1_FN
#define DOUBLE_MATH1_FN(Name)  \
static unsigned builtin_##Name##_enum = -1;
#undef  DOUBLE_MATH2_FN
#define DOUBLE_MATH2_FN(Name)  \
static unsigned builtin_##Name##_enum = -1;
DOUBLE_BUILTINS

//------------------------------------------------------------------------------

inline bool
ast_isApplicate(
	Ast ast
) {
	return ast_isOp(ast, builtin_applicate_enum);
}

inline bool
ast_isTag(
	Ast ast
) {
	return ast_isOp(ast, builtin_tag_enum);
}

inline bool
ast_isTagRef(
	Ast ast
) {
	return ast_isOp(ast, builtin_tag_ref_enum);
}

inline bool
ast_isConst(
	Ast ast
) {
	return ast_isOp(ast, builtin_const_enum);
}

inline bool
ast_isGlobalTag(
	Ast ast
) {
	return ast_isOp(ast, builtin_global_tag_enum);
}

inline bool
ast_isGlobalTagRef(
	Ast ast
) {
	return ast_isOp(ast, builtin_global_tag_ref_enum);
}

inline bool
ast_isGlobalConst(
	Ast ast
) {
	return ast_isOp(ast, builtin_global_const_enum);
}

inline bool
ast_isDeclaration(
	Ast ast
) {
	return ast_isOp(ast, builtin_tag_enum)
		|| ast_isOp(ast, builtin_tag_ref_enum)
		|| ast_isOp(ast, builtin_const_enum)
		|| ast_isOp(ast, builtin_global_tag_enum)
		|| ast_isOp(ast, builtin_global_tag_ref_enum)
		|| ast_isOp(ast, builtin_global_const_enum)
	;
}

inline bool
ast_isAssign(
	Ast ast
) {
	return ast_isOp(ast, builtin_assign_enum);
}

inline bool
ast_isnotAssign(
	Ast ast
) {
	return ast_isnotOp(ast, builtin_assign_enum);
}

inline bool
ast_isArray(
	Ast ast
) {
	return ast_isOp(ast, builtin_array_enum);
}

inline bool
ast_isnotArray(
	Ast ast
) {
	return ast_isnotOp(ast, builtin_array_enum);
}

inline bool
ast_isBracketed(
	Ast ast
) {
	return ast_isOp(ast, builtin_array_enum);
}

inline bool
ast_isnotBracketed(
	Ast ast
) {
	return ast_isnotOp(ast, builtin_array_enum);
}

inline bool
ast_isRange(
	Ast ast
) {
	return ast_isOp(ast, builtin_range_enum);
}

inline bool
ast_isnotRange(
	Ast ast
) {
	return ast_isnotOp(ast, builtin_range_enum);
}

//------------------------------------------------------------------------------

#define TYPE(L,R)  (((L) << TYPE_BIT) | (R))

typedef uint64_t (*IntegerOp)(uint64_t, uint64_t);
#define INTEGEROP(Name,...) \
	uint64_t \
	integer_##Name( \
		uint64_t lval, \
		uint64_t rval  \
	) { \
		__VA_ARGS__; \
	}

typedef double (*FloatFn)(double);
#define FLOATFN(Name,...) \
	double \
	float_##Name( \
		double val  \
	) { \
		__VA_ARGS__; \
	}

typedef double (*FloatOp)(double, double);
#define FLOATOP(Name,...) \
	double \
	float_##Name( \
		double lval, \
		double rval  \
	) { \
		__VA_ARGS__; \
	}

typedef String (*StringOp)(StringConst, StringConst);
#define STRINGOP(Name,...) \
	String \
	string_##Name( \
		StringConst lval, \
		StringConst rval  \
	) { \
		__VA_ARGS__; \
	}

typedef String (*StrIntOp)(StringConst, uint64_t);
#define STRINTOP(Name,...) \
	String \
	string_##Name( \
		StringConst lval, \
		uint64_t    rval  \
	) { \
		__VA_ARGS__; \
	}

typedef Array (*ArrIntOp)(Array, uint64_t);
#define ARRINTOP(Name,...) \
	Array \
	array_##Name( \
		Array    lval, \
		uint64_t rval  \
	) { \
		__VA_ARGS__; \
	}

typedef uint64_t (*IntegerCmp)(uint64_t, uint64_t);
#define INTEGERCMP(Name,...) \
	uint64_t \
	integer_##Name( \
		uint64_t lval, \
		uint64_t rval  \
	) { \
		__VA_ARGS__; \
	}

typedef uint64_t (*FloatCmp)(double, double);
#define FLOATCMP(Name,...) \
	uint64_t \
	float_##Name( \
		double lval, \
		double rval  \
	) { \
		__VA_ARGS__; \
	}

typedef uint64_t (*StringCmp)(StringConst, StringConst);
#define STRINGCMP(Name,...) \
	uint64_t \
	string_##Name( \
		StringConst lval, \
		StringConst rval  \
	) { \
		__VA_ARGS__; \
	}

//------------------------------------------------------------------------------

static inline uint64_t
double_to_uint64_t(
	double v
) {
	return ((union { double d; uint64_t u; }){ .d = v }).u;
}

uint64_t
ast_toInteger(
	Ast ast
) {
	switch(ast_type(ast)) {
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return ast->m.ival;
	case AST_Float:
		return (uint64_t)ast->m.fval;
	case AST_String: {
			char const *cs = StringToCharLiteral(ast->m.sval, NULL);
			return strtou(cs, NULL);
		}
	default:
		return 0;
	}
}

double
ast_toFloat(
	Ast ast
) {
	switch(ast_type(ast)) {
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return (double)ast->m.ival;
	case AST_Float:
		return ast->m.fval;
	case AST_String: {
			char const *cs = StringToCharLiteral(ast->m.sval, NULL);
			return strtod(cs, 0);
		}
	default:
		return 0.0;
	}
}

bool
ast_toBool(
	Ast ast
) {
	switch(ast_type(ast)) {
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return ast->m.ival != 0;
	case AST_Float:
		return !isunordered(ast->m.fval, 0.0) && !islessgreater(ast->m.fval, 0.0);
	case AST_String:
		return StringLength(ast->m.sval) != 0;
	default:
		return false;
	}
}

static inline bool
float_equal(
	double lval,
	double rval
) {
	return !isunordered(lval, rval) && !islessgreater (lval, rval);
}

static bool
are_equal(
	Ast lexpr,
	Ast rexpr
) {
	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return (lexpr->m.ival == rexpr->m.ival);
	case TYPE(AST_Boolean  , AST_Float):
	case TYPE(AST_Integer  , AST_Float):
	case TYPE(AST_Character, AST_Float):
		return float_equal((double)lexpr->m.ival, rexpr->m.fval);
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return float_equal(lexpr->m.fval, (double)rexpr->m.ival);
	case TYPE(AST_Float, AST_Float):
		return float_equal(lexpr->m.fval, rexpr->m.fval);
	case TYPE(AST_String, AST_String):
		return StringEqual(lexpr->m.sval, rexpr->m.sval);
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return (lexpr->m.ival == 0);
	case TYPE(AST_Float, AST_Zen):
		return float_equal(lexpr->m.fval, 0.0);
	case TYPE(AST_String, AST_Zen):
		return StringEqual(lexpr->m.sval, NullString());
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return (0 == rexpr->m.ival);
	case TYPE(AST_Zen, AST_Float):
		return float_equal(0.0, rexpr->m.fval);
	case TYPE(AST_Zen, AST_String):
		return StringEqual(NullString(), rexpr->m.sval);
	case TYPE(AST_Error, AST_Error):
		return (lexpr->qual == rexpr->qual);
	default:
		return false;
	}
}

static int
comparator(
	Ast lexpr,
	Ast rexpr
) {
	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return (lexpr->m.ival > rexpr->m.ival)
			- (lexpr->m.ival < rexpr->m.ival);
	case TYPE(AST_Boolean  , AST_Float):
	case TYPE(AST_Integer  , AST_Float):
	case TYPE(AST_Character, AST_Float):
		return isgreater((double)lexpr->m.ival, rexpr->m.fval)
				- isless((double)lexpr->m.ival, rexpr->m.fval);
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return isgreater(lexpr->m.fval, (double)rexpr->m.ival)
				- isless(lexpr->m.fval, (double)rexpr->m.ival);
	case TYPE(AST_Float, AST_Float):
		return isgreater(lexpr->m.fval, rexpr->m.fval)
				- isless(lexpr->m.fval, rexpr->m.fval);
	case TYPE(AST_String, AST_String):
		return StringCompare(lexpr->m.sval, rexpr->m.sval);
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return (lexpr->m.ival > 0);
	case TYPE(AST_Float, AST_Zen):
		return isgreater(lexpr->m.fval, 0.0);
	case TYPE(AST_String, AST_Zen):
		return StringCompare(lexpr->m.sval, NullString());
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return -(0 < rexpr->m.ival);
	case TYPE(AST_Zen, AST_Float):
		return -isless(0.0, rexpr->m.fval);
	case TYPE(AST_Zen, AST_String):
		return StringCompare(NullString(), rexpr->m.sval);
	case TYPE(AST_Error, AST_Error):
		return (lexpr->qual > rexpr->qual)
			- (lexpr->qual < rexpr->qual);
	default:
		return -1;
	}
}

static bool
in_range_1(
	Ast lexpr,
	Ast expr,
	Ast rexpr
) {
	if(ast_isEnvironment(expr)) {
		Array     arr = expr->m.env;
		int const N   = array_length(arr);

		for(int i = 0; i < N; i++) {
			expr = array_at(arr, Ast, i);

			if(!in_range_1(lexpr, expr, rexpr)) {
				return false;
			}
		}

		return true;
	}

	return (comparator(lexpr, expr) <= 0)
		&& (comparator(expr, rexpr) <= 0)
	;
}

static bool
in_range(
	Ast env,
	Ast range,
	Ast expr
) {
	Ast lexpr = eval(env, range->m.lexpr);
	Ast rexpr = eval(env, range->m.rexpr);

	if(comparator(lexpr, rexpr) < 0) {
		return in_range_1(lexpr, expr, rexpr);
	}

	return in_range_1(rexpr, expr, lexpr);
}

static Ast
invalid_operand(
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	if(ast_isError(lexpr)) {
		return lexpr;
	}

	if(ast_isError(rexpr)) {
		return rexpr;
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_if_1(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	bool   inverted
) {
	if(ast_isZen(lexpr) || ast_isZen(rexpr)) {
		uint64_t cond = ast_toBool(eval(env, ast_isnotZen(lexpr) ? lexpr : rexpr)) ^ inverted;
		return new_ast(sloc, AST_Boolean, cond);
	}

	for(lexpr = undefer(env, lexpr);
		ast_isAssemblage(lexpr);
		lexpr = lexpr->m.rexpr
	) {
		eval(env, lexpr->m.lexpr);
	}

	lexpr = eval(env, lexpr);

	bool cond = ast_toBool(lexpr) ^ inverted;

	rexpr = undefer(env, rexpr);
	if(ast_isAssemblage(rexpr)) {
		if(!cond) {
			return refeval(env, rexpr->m.rexpr);
		}

		return refeval(env, rexpr->m.lexpr);
	}

	if(cond) {
		return refeval(env, rexpr);
	}

	return lexpr;
}

static Ast
builtin_if(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_if_1(env, sloc, lexpr, rexpr, false);
}

static Ast
builtin_ifnot(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_if_1(env, sloc, lexpr, rexpr, true);
}

//------------------------------------------------------------------------------

static bool
builtin_case_match(
	Ast    env,
	Ast    expr
) {
	if(ast_isTag(expr)) {
		if(builtin_case_match(env, expr->m.lexpr)) {
			return true;
		}

		expr = expr->m.rexpr;
	}

	expr = eval(env, expr);

	return ast_toBool(expr);
}

static bool
builtin_case_equal(
	Ast    env,
	Ast    expr,
	Ast    cond
) {
	if(ast_isTag(expr)) {
		if(builtin_case_equal(env, expr->m.lexpr, cond)) {
			return true;
		}

		expr = expr->m.rexpr;
	}

	expr = eval(env, expr);

	if(ast_isRange(expr)) {
		return in_range(env, expr, cond);
	}

	return are_equal(expr, cond);
}

static Ast
builtin_case(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	for(lexpr = undefer(env, lexpr);
		ast_isAssemblage(lexpr);
		lexpr = lexpr->m.rexpr
	) {
		eval(env, lexpr->m.lexpr);
	}

	if(ast_isZen(lexpr)) {
		for(rexpr = undefer(env, rexpr);
			ast_isAssemblage(rexpr);
			rexpr = rexpr->m.rexpr
		) {
			lexpr = rexpr->m.lexpr;

			if(ast_isTag(lexpr)) {
				if(builtin_case_match(env, lexpr->m.lexpr)) {
					return refeval(env, lexpr->m.rexpr);
				}
				continue;
			}

			eval(env, lexpr);
		}

		if(ast_isTag(rexpr)) {
			if(builtin_case_match(env, rexpr->m.lexpr)) {
				return refeval(env, rexpr->m.rexpr);
			}
			return ZEN;
		}

	} else {
		Ast cond = eval(env, lexpr);

		for(rexpr = undefer(env, rexpr);
			ast_isAssemblage(rexpr);
			rexpr = rexpr->m.rexpr
		) {
			lexpr = rexpr->m.lexpr;

			if(ast_isTag(lexpr)) {
				if(builtin_case_equal(env, lexpr->m.lexpr, cond)) {
					return refeval(env, lexpr->m.rexpr);
				}
				continue;
			}

			eval(env, lexpr);
		}

		if(ast_isTag(rexpr)) {
			if(builtin_case_equal(env, rexpr->m.lexpr, cond)) {
				return refeval(env, rexpr->m.rexpr);
			}
			return ZEN;
		}
	}

	return refeval(env, rexpr);
	(void)sloc;
}

//------------------------------------------------------------------------------

static Ast
builtin_tag(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
);

static Ast
builtin_referent_assign(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	By     by
);

static Ast
builtin_loop_array(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	Ast    iexpr
) {
	Ast texpr = ast_isTag(lexpr) ? (
		addenv(env, sloc, lexpr->m.lexpr, ZEN, 0)
	): ast_isConst(lexpr) ? (
		addenv(env, sloc, lexpr->m.lexpr, ZEN, ATTR_NoAssign)
	):(
		unwrapref(subeval(env, lexpr->m.lexpr))
	);
	if(ast_isReference(texpr)) {

		Ast result = ZEN;

		Ast   ast = eval(env, iexpr->m.lexpr);
		Array arr;
		if(ast_isZen(ast)) {
			ast   = eval(env, iexpr);
			arr   = ast->m.env;
			iexpr = ZEN;
		} else {
			arr   = ast->m.env;
			iexpr = eval(env, iexpr->m.rexpr);
		}

		size_t const length = array_length(arr);
		size_t const last   = length - !!length;
		size_t       index  = 0;
		size_t       end    = last;
		size_t       step   = 1;
		if(ast_isRange(iexpr)) {
			index = ast_toInteger(eval(env, iexpr->m.lexpr));
			ast   = eval(env, iexpr->m.rexpr);
			end   = ast_isZen(ast) ? last : ast_toInteger(ast);
			if(index > end) {
				if(index > last) {
					if(end > last) {
						return oboerr(sloc, ERR_InvalidOperand);
					}

					index = last;
				}
				if(index > end) {
					step = -1;
				}
			} else {
				if(index > last) {
					return oboerr(sloc, ERR_InvalidOperand);
				}
				if(end > last) {
					end = last;
				}
			}
		}

		if(length > 0) for(size_t ts = gc_topof_stack();;) {
			texpr->m.rexpr = array_at(arr, Ast, index);

			result = refeval(env, rexpr);

			texpr->m.rexpr = ZEN;

			gc_return(ts, result);

			if(index == end) break;
			index += step;
		}

		return result;
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

static Ast
builtin_loop_range(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	Ast    iexpr
) {
	Ast texpr = ast_isTag(lexpr) ? (
		addenv(env, sloc, lexpr->m.lexpr, ZEN, 0)
	): ast_isConst(lexpr) ? (
		addenv(env, sloc, lexpr->m.lexpr, ZEN, ATTR_NoAssign)
	):(
		unwrapref(subeval(env, lexpr->m.lexpr))
	);
	if(ast_isReference(texpr)) {

		Ast result = ZEN;

		uint64_t next = ast_toInteger(eval(env, iexpr->m.lexpr));
		Ast      ast  = eval(env, iexpr->m.rexpr);
		uint64_t end  = ast_isZen(ast) ? UINT64_MAX : ast_toInteger(ast);
		uint64_t step = (next > end) ? -1 : 1;

		iexpr = new_ast(sloc, AST_Integer, next);

		for(size_t ts = gc_topof_stack();;) {
			texpr->m.rexpr = iexpr;

			result = refeval(env, rexpr);

			gc_return(ts, result);

			if(next == end) break;

			next += step;
			iexpr->m.ival = next;
		}

		return result;
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

static Ast
builtin_loop_1(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	bool   inverted
) {
	Ast iexpr = ZEN;

	lexpr = unquote(lexpr);
	if((ast_isTag(lexpr)
			|| ast_isConst(lexpr)
			|| ast_isAssign(lexpr))
		&& ast_isIdentifier(lexpr->m.lexpr)
	) {
		if(ast_isArray(lexpr->m.rexpr)) {
			if(ast_isZen(lexpr->m.rexpr->m.lexpr)
				|| ast_isZen(lexpr->m.rexpr->m.rexpr)
				|| ast_isRange(lexpr->m.rexpr->m.rexpr)
			) {
				return builtin_loop_array(env, sloc, lexpr, rexpr, lexpr->m.rexpr);
			}
		} else {
			iexpr = undefer(env, lexpr->m.rexpr);

			if(ast_isRange(iexpr)) {
				return builtin_loop_range(env, sloc, lexpr, rexpr, iexpr);
			}

			iexpr = ZEN;
		}
	}

	lexpr = undefer(env, lexpr);
	if(ast_isAssemblage(lexpr)) {
		if(ast_isAssemblage(lexpr->m.rexpr)) {
			do {
				eval(env, lexpr->m.lexpr);
				lexpr = lexpr->m.rexpr;
			} while(ast_isAssemblage(lexpr->m.rexpr))
				;
			iexpr = lexpr->m.rexpr;
			lexpr = lexpr->m.lexpr;

		} else {
			eval(env, lexpr->m.lexpr);
			lexpr = lexpr->m.rexpr;
		}
	}

	bool cond = ast_toBool(eval(env, lexpr)) ^ inverted;

	rexpr = undefer(env, rexpr);
	if(ast_isAssemblage(rexpr)) {
		if(!cond) {
			return eval(env, rexpr->m.rexpr);
		}

		rexpr = rexpr->m.lexpr;
	}

	size_t ts     = gc_topof_stack();
	Ast    result = ZEN;

	if(ast_isnotZen(iexpr)) {
		while(cond) {
			result = refeval(env, rexpr);

			eval(env, iexpr);

			cond = ast_toBool(eval(env, lexpr)) ^ inverted;

			gc_return(ts, result);
		}
	} else {
		while(cond) {
			result = refeval(env, rexpr);

			cond = ast_toBool(eval(env, lexpr)) ^ inverted;

			gc_return(ts, result);
		}
	}

	return result;
	(void)sloc;
}

static Ast
builtin_while(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_loop_1(env, sloc, lexpr, rexpr, false);
}

static Ast
builtin_until(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_loop_1(env, sloc, lexpr, rexpr, true);
}

#define BUILTIN_CONDITION

//------------------------------------------------------------------------------

static Ast
builtin_land(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	uint64_t result;

	lexpr  = eval(env, lexpr);
	result = ast_toBool(lexpr);
	if(result) {
		rexpr  = eval(env, rexpr);
		result = ast_toBool(rexpr);
	}

	return new_ast(sloc, AST_Boolean, result);
}

static Ast
builtin_lor(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	uint64_t result;

	lexpr  = eval(env, lexpr);
	result = ast_toBool(lexpr);
	if(!result) {
		rexpr  = eval(env, rexpr);
		result = ast_toBool(rexpr);
	}

	return new_ast(sloc, AST_Boolean, result);
}

//------------------------------------------------------------------------------

static int
compare_delegate(
	Ast        env,
	sloc_t     sloc,
	Ast        lexpr,
	Ast        rexpr,
	IntegerCmp integercmp,
	FloatCmp   floatcmp,
	StringCmp  stringcmp,
	int        sense
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	if(ast_isEnvironment(lexpr) || ast_isEnvironment(rexpr)) {
		if(ast_isEnvironment(lexpr) && ast_isEnvironment(rexpr)) {
			size_t const ln = array_length(lexpr->m.env);
			size_t const rn = array_length(rexpr->m.env);
			size_t const n = minz(ln, rn);
			int          r = 1;
			for(size_t i = 0; r > 0 && i < n; i++) {
				r = compare_delegate(env, sloc,
						array_at(lexpr->m.env, Ast, i),
						array_at(rexpr->m.env, Ast, i),
						integercmp,
						floatcmp,
						stringcmp,
						sense
					);
			}
			for(size_t i = n; r == sense && i < ln; i++) {
				r = compare_delegate(env, sloc,
						array_at(lexpr->m.env, Ast, i),
						ZEN,
						integercmp,
						floatcmp,
						stringcmp,
						sense
					);
			}
			for(size_t i = n; r == sense && i < rn; i++) {
				r = compare_delegate(env, sloc,
						ZEN,
						array_at(rexpr->m.env, Ast, i),
						integercmp,
						floatcmp,
						stringcmp,
						sense
					);
			}
			return r;
		} else if(ast_isEnvironment(lexpr)) {
			size_t const n = array_length(lexpr->m.env);
			int          r = 1;
			for(size_t i = 0; r > 0 && i < n; i++) {
				r = compare_delegate(env, sloc,
						array_at(lexpr->m.env, Ast, i),
						rexpr,
						integercmp,
						floatcmp,
						stringcmp,
						sense
					);
			}
			return r;
		} else {
			size_t const n = array_length(rexpr->m.env);
			int          r = 1;
			for(size_t i = 0; r > 0 && i < n; i++) {
				r = compare_delegate(env, sloc,
						lexpr,
						array_at(rexpr->m.env, Ast, i),
						integercmp,
						floatcmp,
						stringcmp,
						sense
					);
			}
			return r;
		}

	} else switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return integercmp(lexpr->m.ival, rexpr->m.ival);
	case TYPE(AST_Boolean, AST_Float):
	case TYPE(AST_Integer, AST_Float):
	case TYPE(AST_Character, AST_Float):
		return floatcmp((double)lexpr->m.ival, rexpr->m.fval);
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return floatcmp(lexpr->m.fval, (double)rexpr->m.ival);
	case TYPE(AST_Float, AST_Float):
		return floatcmp(lexpr->m.fval, rexpr->m.fval);
	case TYPE(AST_String, AST_String):
		return stringcmp(lexpr->m.sval, rexpr->m.sval);
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return integercmp(lexpr->m.ival, 0);
	case TYPE(AST_Float, AST_Zen):
		return floatcmp(lexpr->m.fval, 0.0);
	case TYPE(AST_String, AST_Zen):
		return stringcmp(lexpr->m.sval, NullString());
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return integercmp(0, rexpr->m.ival);
	case TYPE(AST_Zen, AST_Float):
		return floatcmp(0.0, rexpr->m.fval);
	case TYPE(AST_Zen, AST_String):
		return stringcmp(NullString(), rexpr->m.sval);
	case TYPE(AST_Error, AST_Error):
		return integercmp(lexpr->qual, rexpr->qual);
	default:
		return -1;
	}
}

static Ast
builtin_compare(
	Ast        env,
	sloc_t     sloc,
	Ast        lexpr,
	Ast        rexpr,
	IntegerCmp integercmp,
	FloatCmp   floatcmp,
	StringCmp  stringcmp,
	int        sense
) {
	int const r = compare_delegate(env, sloc,
		lexpr, rexpr,
		integercmp, floatcmp, stringcmp,
		sense
	);
	if(r >= 0) {
		return new_ast(sloc, AST_Boolean, (uint64_t)(r != 0));
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

#define BUILTIN_COMPARE(Name, Sense) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_compare(env, sloc, lexpr, rexpr, integer_##Name, float_##Name, string_##Name, Sense); \
}

static INTEGERCMP(lt,  return lval <  rval)
static INTEGERCMP(lte, return lval <= rval)
static INTEGERCMP(eq,  return lval == rval)
static INTEGERCMP(neq, return lval != rval)
static INTEGERCMP(gte, return lval >= rval)
static INTEGERCMP(gt,  return lval >  rval)

static FLOATCMP(lt,    return !isunordered(lval, rval) &&  isless        (lval, rval))
static FLOATCMP(lte,   return !isunordered(lval, rval) &&  islessequal   (lval, rval))
static FLOATCMP(eq,    return !isunordered(lval, rval) && !islessgreater (lval, rval))
static FLOATCMP(neq,   return !isunordered(lval, rval) &&  islessgreater (lval, rval))
static FLOATCMP(gte,   return !isunordered(lval, rval) &&  isgreaterequal(lval, rval))
static FLOATCMP(gt,    return !isunordered(lval, rval) &&  isgreater     (lval, rval))

static STRINGCMP(lt,   return (StringCompare(lval, rval) <  0))
static STRINGCMP(lte,  return (StringCompare(lval, rval) <= 0))
static STRINGCMP(eq,   return StringEqual   (lval, rval))
static STRINGCMP(neq,  return StringNotEqual(lval, rval))
static STRINGCMP(gte,  return (StringCompare(lval, rval) >= 0))
static STRINGCMP(gt,   return (StringCompare(lval, rval) >  0))

static BUILTIN_COMPARE(lt, 0)
static BUILTIN_COMPARE(lte, 1)
static BUILTIN_COMPARE(eq, 1)
static BUILTIN_COMPARE(neq, 0)
static BUILTIN_COMPARE(gte, 1)
static BUILTIN_COMPARE(gt, 0)

//------------------------------------------------------------------------------

static Ast
builtin_bitwise(
	Ast       env,
	sloc_t    sloc,
	Ast       lexpr,
	Ast       rexpr,
	IntegerOp integerop
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Boolean, AST_Float):
	case TYPE(AST_Integer, AST_Float):
	case TYPE(AST_Character, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, double_to_uint64_t(rexpr->m.fval)));
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), double_to_uint64_t(rexpr->m.fval)));
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), 0));
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(0, double_to_uint64_t(rexpr->m.fval)));
	default:
		return invalid_operand(sloc, lexpr, rexpr);
	}
}
#define BUILTIN_BITWISE(Name) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_bitwise(env, sloc, lexpr, rexpr, integer_##Name); \
}

static INTEGEROP(and, return lval & rval)
static INTEGEROP(or , return lval | rval)
static INTEGEROP(xor, return lval ^ rval)

static BUILTIN_BITWISE(and)
static BUILTIN_BITWISE(or)
static BUILTIN_BITWISE(xor)

//------------------------------------------------------------------------------

static Ast
builtin_arithmetic(
	Ast       env,
	sloc_t    sloc,
	Ast       lexpr,
	Ast       rexpr,
	IntegerOp integerop,
	FloatOp   floatop
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Boolean, AST_Float):
	case TYPE(AST_Integer, AST_Float):
	case TYPE(AST_Character, AST_Float):
		return new_ast(sloc, AST_Float, floatop((double)lexpr->m.ival, rexpr->m.fval));
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return new_ast(sloc, AST_Float, floatop(lexpr->m.fval, (double)rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, AST_Float, floatop(lexpr->m.fval, rexpr->m.fval));
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, AST_Float, floatop(lexpr->m.fval, 0.0));
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, AST_Float, floatop(0.0, rexpr->m.fval));
	default:
		return invalid_operand(sloc, lexpr, rexpr);
	}
}
#define BUILTIN_ARITHMETIC(Name) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_arithmetic(env, sloc, lexpr, rexpr, integer_##Name, float_##Name); \
}

static INTEGEROP(add, return lval + rval)
static INTEGEROP(sub, return lval - rval)
static INTEGEROP(mul, return lval * rval)
static INTEGEROP(div, return (rval != 0) ? lval / rval : 0)
static INTEGEROP(mod, return (rval != 0) ? lval % rval : 0)

static FLOATOP(add, return !isunordered(lval, rval) ? lval + rval : 0.0)
static FLOATOP(sub, return !isunordered(lval, rval) ? lval - rval : 0.0)
static FLOATOP(mul, return !isunordered(lval, rval) ? lval * rval : 0.0)
static FLOATOP(div, return !isunordered(lval, rval) ? (islessgreater(rval, 0.0) ? lval / rval : 0.0) : 0.0)
static FLOATOP(mod, return !isunordered(lval, rval) ? (islessgreater(rval, 0.0) ? fmod(lval, rval) : 0.0) : 0.0)

static BUILTIN_ARITHMETIC(add)
static BUILTIN_ARITHMETIC(sub)
static BUILTIN_ARITHMETIC(mul)
static BUILTIN_ARITHMETIC(div)
static BUILTIN_ARITHMETIC(mod)

//------------------------------------------------------------------------------

static Ast
builtin_bitmove(
	Ast       env,
	sloc_t    sloc,
	Ast       lexpr,
	Ast       rexpr,
	IntegerOp integerop,
	StrIntOp  stringop,
	ArrIntOp  arrayop
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Boolean  , AST_Boolean):
	case TYPE(AST_Boolean  , AST_Integer):
	case TYPE(AST_Boolean  , AST_Character):
	case TYPE(AST_Integer  , AST_Boolean):
	case TYPE(AST_Integer  , AST_Integer):
	case TYPE(AST_Integer  , AST_Character):
	case TYPE(AST_Character, AST_Boolean):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Boolean, AST_Float):
	case TYPE(AST_Integer, AST_Float):
	case TYPE(AST_Character, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, (uint64_t)rexpr->m.fval));
	case TYPE(AST_Float, AST_Boolean):
	case TYPE(AST_Float, AST_Integer):
	case TYPE(AST_Float, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), (uint64_t)rexpr->m.fval));
	case TYPE(AST_String, AST_Boolean):
	case TYPE(AST_String, AST_Integer):
	case TYPE(AST_String, AST_Character):
		return new_ast(sloc, AST_String , stringop(lexpr->m.sval, rexpr->m.ival));
	case TYPE(AST_String, AST_Float):
		return new_ast(sloc, AST_String , stringop(lexpr->m.sval, (uint64_t)rexpr->m.fval));
	case TYPE(AST_Environment, AST_Boolean):
	case TYPE(AST_Environment, AST_Integer):
	case TYPE(AST_Environment, AST_Character):
		return new_ast(sloc, AST_Environment, arrayop(lexpr->m.env, rexpr->m.ival), ZEN);
	case TYPE(AST_Environment, AST_Float):
		return new_ast(sloc, AST_Environment, arrayop(lexpr->m.env, (uint64_t)rexpr->m.fval), ZEN);
	case TYPE(AST_Boolean, AST_Zen):
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), 0));
	case TYPE(AST_String, AST_Zen):
		return new_ast(sloc, AST_String , stringop(lexpr->m.sval, 0));
	case TYPE(AST_Zen, AST_Boolean):
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, AST_Integer, integerop(0, (uint64_t)rexpr->m.fval));
	default:
		return invalid_operand(sloc, lexpr, rexpr);
	}
}
#define BUILTIN_BITMOVE(Name) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_bitmove(env, sloc, lexpr, rexpr, integer_##Name, string_##Name, array_##Name); \
}

static INTEGEROP(shl, rval &= 63; return lval << rval)
static INTEGEROP(shr, rval &= 63; return lval >> rval)
static INTEGEROP(exl, rval &= 63; return lval >> (64 - rval))
static INTEGEROP(exr, rval &= 63; return lval  & (UINT64_C(~0) >> (64 - rval)))
static INTEGEROP(rol, rval &= 63; return (lval << rval) | (lval >> (64 - rval)))
static INTEGEROP(ror, rval &= 63; return (lval << (64 - rval)) | (lval >> rval))

static STRINTOP (shl,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = codepointoffset(cs, n, rval);
	return CharLiteralToString(cs + rval, n - rval);
)
static STRINTOP (shr,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = reversecodepointoffset(cs, n, rval);
	return CharLiteralToString(cs, rval);
)
static STRINTOP (exl,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = codepointoffset(cs, n, rval);
	return CharLiteralToString(cs, rval);
)
static STRINTOP (exr,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = reversecodepointoffset(cs, n, rval);
	return CharLiteralToString(cs + rval, n - rval);
)
static STRINTOP (rol,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = codepointoffset(cs, n, rval);
	return CharLiteralsToString(cs + rval, n - rval, cs, rval);
)
static STRINTOP (ror,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval = reversecodepointoffset(cs, n, rval);
	return CharLiteralsToString(cs + rval, n - rval, cs, rval);
)

static inline Array
arrintop_alloc(
	void
) {
	Array arr = gc_malloc(sizeof(*arr), env_gc_mark, env_gc_sweep);
	assert(arr != NULL);
	*arr = ARRAY();
	return arr;
}
static inline void
arrintop_resize(
	Array arr,
	size_t n
) {
	bool   expanded = array_expand(arr, sizeof(Ast), n);
	assert(expanded);
	arr->length = n;
	return;
}
static inline void
arrintop_copy(
	Array  t,
	size_t ti,
	Array  s,
	size_t si,
	size_t n
) {
	for(; n-- > 0; si++, ti++) {
		Ast st = array_at(s, Ast, si);
		st     = dup_ast(st->sloc, st);
		array_at(t, Ast, ti) = st;
		if(ast_isReference(st)) {
			size_t      len;
			char const *cs = StringToCharLiteral(st->m.sval, &len);
			uint64_t    h  = memhash(cs, len, 0);
			size_t      x  = array_map_index(t, h, ti);
			assert(x == ti);
		}
	}

	return;
}
static ARRINTOP (shl,
	Array arr = arrintop_alloc();
	if(array_length(lval) > rval) {
		size_t n = (array_length(lval) - rval);
		arrintop_resize(arr, n);
		arrintop_copy(arr, 0, lval, rval, n);
	}
	return arr;
)
static ARRINTOP (shr,
	Array arr = arrintop_alloc();
	if(array_length(lval) > rval) {
		size_t n = (array_length(lval) - rval);
		arrintop_resize(arr, n);
		arrintop_copy(arr, 0, lval, 0, n);
	}
	return arr;
)
static ARRINTOP (exl,
	Array arr = arrintop_alloc();
	if(rval > array_length(lval)) {
		rval = array_length(lval);
	}
	if(array_length(lval) >= rval) {
		arrintop_resize(arr, rval);
		arrintop_copy(arr, 0, lval, 0, rval);
	}
	return arr;
)
static ARRINTOP (exr,
	Array arr = arrintop_alloc();
	if(rval > array_length(lval)) {
		rval = array_length(lval);
	}
	if(array_length(lval) >= rval) {
		size_t n = (array_length(lval) - rval);
		arrintop_resize(arr, rval);
		arrintop_copy(arr, 0, lval, n, rval);
	}
	return arr;
)
static ARRINTOP (rol,
	Array arr = arrintop_alloc();
	if(array_length(lval) > 0) {
		rval = rval % array_length(lval);
	}
	if(array_length(lval) > rval) {
		size_t n = (array_length(lval) - rval);
		arrintop_resize(arr, array_length(lval));
		arrintop_copy(arr, 0, lval, rval, n);
		arrintop_copy(arr, n, lval, 0, rval);
	}
	return arr;
)
static ARRINTOP (ror,
	Array arr = arrintop_alloc();
	if(array_length(lval) > 0) {
		rval = rval % array_length(lval);
	}
	if(array_length(lval) > rval) {
		size_t n = (array_length(lval) - rval);
		arrintop_resize(arr, array_length(lval));
		arrintop_copy(arr, 0, lval, n, rval);
		arrintop_copy(arr, rval, lval, 0, n);
	}
	return arr;
)

static BUILTIN_BITMOVE(shl)
static BUILTIN_BITMOVE(shr)
static BUILTIN_BITMOVE(exl)
static BUILTIN_BITMOVE(exr)
static BUILTIN_BITMOVE(rol)
static BUILTIN_BITMOVE(ror)

//------------------------------------------------------------------------------

static Ast
builtin_math1(
	Ast     env,
	sloc_t  sloc,
	Ast     arg,
	FloatFn floatfn
) {
	arg = eval(env, arg);

	switch(ast_type(arg)) {
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Float, floatfn((double)arg->m.ival));
	case AST_Float:
		return new_ast(sloc, AST_Float, floatfn(arg->m.fval));
	case AST_Error:
		return arg;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}
}

static Ast
builtin_math2(
	Ast     env,
	sloc_t  sloc,
	Ast     arg,
	FloatOp floatop
) {
	if(ast_isSequence(arg)) {
		Ast lexpr = eval(env, arg->m.lexpr);
		Ast rexpr = eval(env, arg->m.rexpr);

		switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
		case TYPE(AST_Boolean  , AST_Boolean):
		case TYPE(AST_Boolean  , AST_Integer):
		case TYPE(AST_Boolean  , AST_Character):
		case TYPE(AST_Integer  , AST_Boolean):
		case TYPE(AST_Integer  , AST_Integer):
		case TYPE(AST_Integer  , AST_Character):
		case TYPE(AST_Character, AST_Boolean):
		case TYPE(AST_Character, AST_Integer):
		case TYPE(AST_Character, AST_Character):
			return new_ast(sloc, AST_Float, floatop((double)lexpr->m.ival, (double)rexpr->m.ival));
		case TYPE(AST_Boolean, AST_Float):
		case TYPE(AST_Integer, AST_Float):
		case TYPE(AST_Character, AST_Float):
			return new_ast(sloc, AST_Float, floatop((double)lexpr->m.ival, rexpr->m.fval));
		case TYPE(AST_Float, AST_Boolean):
		case TYPE(AST_Float, AST_Integer):
		case TYPE(AST_Float, AST_Character):
			return new_ast(sloc, AST_Float, floatop(lexpr->m.fval, (double)rexpr->m.ival));
		case TYPE(AST_Float, AST_Float):
			return new_ast(sloc, AST_Float, floatop(lexpr->m.fval, rexpr->m.fval));
		default:
			return invalid_operand(sloc, lexpr, rexpr);
		}
	}

	return oboerr(arg->sloc, ERR_InvalidOperand);
}

#undef  DOUBLE_MATH1_FN
#define DOUBLE_MATH1_FN(Name)  \
static Ast \
builtin_##Name( \
	Ast    env,  \
	sloc_t sloc, \
	Ast    arg   \
) { \
	return builtin_math1(env, sloc, arg, Name); \
}
#undef  DOUBLE_MATH2_FN
#define DOUBLE_MATH2_FN(Name)  \
static Ast \
builtin_##Name( \
	Ast    env,  \
	sloc_t sloc, \
	Ast    arg   \
) { \
	return builtin_math2(env, sloc, arg, Name); \
}
DOUBLE_BUILTINS

//------------------------------------------------------------------------------

static inline Ast
byrefeval(
	Ast env,
	Ast expr
) {
	expr = subeval(env, expr);

	if(ast_isReference(expr)) {
		expr = unwrapref(expr);
	}

	return expr;
}

static inline Ast
evaluate_instance(
	Ast    env,
	sloc_t sloc,
	Ast    expr,
	By     by
) {
	return (by == BY_Value) ?
		dup_ast(sloc, refeval(env, expr))
	:
		dup_ref(sloc, byrefeval(env, expr))
	;
}

static inline Ast
evaluate_assignable(
	Ast    env,
	sloc_t sloc,
	Ast    expr,
	By     by
) {
	return (by == BY_Value) ?
		refeval(env, expr)
	:
		byrefeval(env, expr)
	;
	(void)sloc;
}

//------------------------------------------------------------------------------

static inline bool
ast_isParameter(
	Ast ast
) {
	if(ast_isTag(ast)) {
		ast = ast->m.lexpr;
	}
	return ast_isIdentifier(ast);
}

static bool
ast_isParameters(
	Ast ast
) {
	switch(ast_type(ast)) {
	case AST_Zen:
		return true;
	case AST_Sequence:
		for(;
			(ast_isSequence(ast)
				 && ast_isParameter(ast->m.lexpr)
			);
			ast = ast->m.rexpr
		);
		nobreak;
	default:
		return ast_isParameter(ast);
	}
}

static inline Ast
operator_alias(
	Ast      env,
	sloc_t   sloc,
	String   s,
	uint64_t hash,
	Ast      rexpr
) {
	rexpr             = new_ast(sloc, AST_OperatorAlias, s, rexpr->m.sval);
	size_t      index = define(env, hash, rexpr, ATTR_NoAssign);
	assert(~index != 0);
	return rexpr;
}

static Ast
builtin_decl(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	bool   is_const,
	bool   is_global
) {
	lexpr = unquote(lexpr);

	if(ast_isApplicate(lexpr)) {
		// Function
		if(ast_isIdentifier(lexpr->m.lexpr)
			&& ast_isParameters(lexpr->m.rexpr)
		) {
			rexpr = new_ast(sloc, AST_Function, lexpr->m.rexpr, rexpr);
			addenv((is_global ? globals : env), sloc, lexpr->m.lexpr, rexpr, (is_const ? ATTR_NoAssign : 0));
			return rexpr;
		}
		// OperatorFunction with precedence
		if(ast_isApplicate(lexpr->m.lexpr)
			&& ast_isString(lexpr->m.lexpr->m.lexpr)
			&& ast_isString(lexpr->m.lexpr->m.rexpr)
			&& ast_isParameters(lexpr->m.rexpr)
		) {
			size_t      len;
			char const *cs    = StringToCharLiteral(lexpr->m.lexpr->m.lexpr->m.sval, &len);
			Precedence  prec  = precedence(cs, len);
			String      s     = lexpr->m.lexpr->m.rexpr->m.sval;
			uint64_t    hash  = lexpr->m.lexpr->m.rexpr->m.hash;
			rexpr             = new_ast(sloc, AST_Function, lexpr->m.rexpr, rexpr);
			rexpr             = new_ast(sloc, AST_OperatorFunction, s, rexpr, prec);
			size_t      index = define(operators, hash, rexpr, (is_const ? ATTR_NoAssign : 0));
			assert(~index != 0);
			return rexpr;
		}
		// OperatorFunction
		if(ast_isString(lexpr->m.lexpr)
			&& ast_isParameters(lexpr->m.rexpr)
		) {
			String      s     = lexpr->m.lexpr->m.sval;
			uint64_t    hash  = lexpr->m.lexpr->m.hash;
			rexpr             = new_ast(sloc, AST_Function, lexpr->m.rexpr, rexpr);
			rexpr             = new_ast(sloc, AST_OperatorFunction, s, rexpr, P_Assigning);
			size_t      index = define(operators, hash, rexpr, ATTR_NoAssign);
			assert(~index != 0);
			return rexpr;
		}
		return oboerr(sloc, ERR_InvalidOperand);
	}

	switch(ast_type(lexpr)) {
	case AST_Zen:
		if(is_const) {
			rexpr = ast_isnotQuoted(rexpr) ? (
				new_ast(sloc, AST_Quoted, rexpr)
			): ast_isReference(rexpr) ? (
				dup_ref(sloc, rexpr)
			):(
				dup_ast(sloc, rexpr)
			);
			rexpr->attr |= ATTR_NoAssign;
			return rexpr;
		}
		return ast_isnotQuoted(rexpr) ? (
			new_ast(sloc, AST_Quoted, rexpr)
		) : (
			rexpr
		);
	case AST_Identifier:
		rexpr = evaluate_instance(env, sloc, rexpr, BY_Value);
		rexpr = addenv((is_global ? globals : env), sloc, lexpr, rexpr, (is_const ? ATTR_NoAssign : 0));
		return rexpr;
	case AST_String: // for OperatorAlias
		if(ast_isString(rexpr)) {
			rexpr = operator_alias(operators, sloc, lexpr->m.sval, lexpr->m.hash, rexpr);
			return rexpr;
		}
		nobreak;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}
}

static Ast
builtin_decl_ref(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	bool   is_const,
	bool   is_global
) {
	lexpr = unquote(lexpr);

	switch(ast_type(lexpr)) {
	case AST_Identifier:
		rexpr = subeval(env, rexpr);
		if(ast_isReference(rexpr)) {
			if(is_const) {
				rexpr = dup_ref(sloc, rexpr);
			}
			addenv((is_global ? globals : env), sloc, lexpr, rexpr, (is_const ? ATTR_NoAssign : 0));
			return rexpr;
		}
		nobreak;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}
}

static inline Ast
builtin_tag(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl(env, sloc, lexpr, rexpr, false, false);
}

static inline Ast
builtin_tag_ref(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl_ref(env, sloc, lexpr, rexpr, false, false);
}

static inline Ast
builtin_const(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl(env, sloc, lexpr, rexpr, true, false);
}

static inline Ast
builtin_global_tag(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl(env, sloc, lexpr, rexpr, false, true);
}

static inline Ast
builtin_global_tag_ref(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl_ref(env, sloc, lexpr, rexpr, false, true);
}

static inline Ast
builtin_global_const(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_decl(env, sloc, lexpr, rexpr, true, true);
}

//------------------------------------------------------------------------------

static Ast
builtin_array_push_back(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	By     by
) {
	if(ast_isTag(rexpr) || ast_isTagRef(rexpr) || ast_isConst(rexpr)) {
		env = link_env(sloc, lexpr, env);

		if(ast_isTag(rexpr)) {
			return builtin_tag(env, sloc, rexpr->m.lexpr, rexpr->m.rexpr);
		}
		if(ast_isTagRef(rexpr)) {
			return builtin_tag_ref(env, sloc, rexpr->m.lexpr, rexpr->m.rexpr);
		}

		return builtin_const(env, sloc, rexpr->m.lexpr, rexpr->m.rexpr);

	} else {
		rexpr = evaluate_instance(env, sloc, rexpr, by);
		bool appended = array_push_back(lexpr->m.env, Ast, rexpr);
		assert(appended);
		return rexpr;
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_array_assign_index(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	size_t index,
	By     by
) {
	Ast *ent = array_ptr(lexpr->m.env, Ast, index);

	rexpr = evaluate_instance(env, sloc, rexpr, by);
	lexpr = *ent;
	if(ast_isReference(lexpr)) {
		if(!ast_isAssignable(lexpr)) {
			return oboerr(sloc, ERR_InvalidReferent);
		}

		ent = &lexpr->m.rexpr;
	}

	assign(sloc, ent, rexpr);
	return rexpr;
}

static Ast
builtin_array_create_map(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	Ast    iexpr,
	By     by
) {
	rexpr = evaluate_instance(env, sloc, rexpr, by);
	rexpr = addenv(lexpr, sloc, iexpr, rexpr, 0);
	if(ast_isnotZen(rexpr)) {
		rexpr = rexpr->m.rexpr;
	}
	return rexpr;
}

static Ast
builtin_referent_assign(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	By     by
) {
	lexpr = unwrapref(lexpr);

	if(ast_isnotZen(lexpr->m.rexpr)) {
		if(ast_isAssignable(lexpr->m.rexpr)) {
			rexpr = evaluate_assignable(env, sloc, rexpr, by);
			rexpr = assign(sloc, &lexpr->m.rexpr, rexpr);
			return rexpr;
		}

		return oboerr(sloc, ERR_InvalidReferent);
	}

	rexpr = evaluate_instance(env, sloc, rexpr, by);
	lexpr->m.rexpr = rexpr;
	return rexpr;
}

//------------------------------------------------------------------------------

static Ast
builtin_assign_by_delegate(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	By     by
) {
	if(ast_isArray(lexpr)) {
		Ast iexpr = eval(env, lexpr->m.rexpr);
		lexpr     = eval(env, lexpr->m.lexpr);

		switch(TYPE(ast_type(lexpr), ast_type(iexpr))) {
		case TYPE(AST_Environment, AST_Boolean):
		case TYPE(AST_Environment, AST_Integer):
		case TYPE(AST_Environment, AST_Character):
			if(ast_isAssignable(lexpr)) {
				size_t const index  = iexpr->m.ival;
				size_t const length = array_length(lexpr->m.env);
				if(index < length) {
					return builtin_array_assign_index(env, sloc, lexpr, rexpr, index, by);
				}
				if(index == length) {
					return builtin_array_push_back(env, sloc, lexpr, rexpr, by);
				}
				return oboerr(sloc, ERR_InvalidOperand);
			}
			return oboerr(sloc, ERR_InvalidReferent);
		case TYPE(AST_Environment, AST_String):
			if(ast_isAssignable(lexpr)) {
				size_t const index  = atenv(lexpr, iexpr);
				size_t const length = array_length(lexpr->m.env);
				if(index < length) {
					return builtin_array_assign_index(env, sloc, lexpr, rexpr, index, by);
				}
				return builtin_array_create_map(env, sloc, lexpr, rexpr, iexpr, by);
			}
			return oboerr(sloc, ERR_InvalidReferent);
		default:
			if(ast_isZen(lexpr) && ast_isRange(iexpr)) {
				lexpr = new_env(sloc, NULL);
			}

			if(ast_isAssignable(lexpr)
				&& ast_isEnvironment(lexpr) && ast_isRange(iexpr)
			) {
				size_t const length = array_length(lexpr->m.env);
				size_t       index  = ast_toInteger(eval(env, iexpr->m.lexpr));
				Ast          ast    = eval(env, iexpr->m.rexpr);
				size_t       end    = ast_isZen(ast) ? length - !!length : ast_toInteger(ast);
				if(index > end) {
					size_t temp = index;
					index       = end;
					end         = temp;
				}
				if(index < length) {
					if(end < length) {
						do {
							builtin_array_assign_index(env, sloc, lexpr, rexpr, index, by);
						} while(index++ != end)
							;
						return lexpr;
					}
					while(index != length) {
						builtin_array_assign_index(env, sloc, lexpr, rexpr, index, by);
						index++;
					}
				}
				if(index == length) {
					do {
						builtin_array_push_back(env, sloc, lexpr, rexpr, by);
					} while(index++ != end)
						;
					return lexpr;
				}
				return oboerr(sloc, ERR_InvalidOperand);
			}
			return oboerr(sloc, ERR_InvalidReferent);
		}
	}

	lexpr = subeval(env, lexpr);

	if(ast_isReference(lexpr) && ast_isAssignable(lexpr)) {
		return builtin_referent_assign(env, sloc, lexpr, rexpr, by);
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

static Ast
builtin_assign_by(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	By     by
) {
	lexpr = unquote(lexpr);

	if(ast_isnotZen(lexpr)) {
		return builtin_assign_by_delegate(env, sloc, lexpr, rexpr, by);
	}

	if(by == BY_Value) {
		rexpr = eval(env, rexpr);
		lexpr = dup_ast(sloc, rexpr);
		return lexpr;
	}

	rexpr = byrefeval(env, rexpr);
	if(ast_isReference(rexpr)) {
		lexpr = dup_ref(sloc, rexpr);
		return lexpr;
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

static Ast
builtin_assign_ref(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_assign_by(env, sloc, lexpr, rexpr, BY_Ref);
}

static Ast
builtin_assign(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	return builtin_assign_by(env, sloc, lexpr, rexpr, BY_Value);
}

static inline Ast
builtin_assign_op(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr,
	Ast  (*builtin)(
		Ast    env,
		sloc_t sloc,
		Ast    lexpr,
		Ast    rexpr
	)
) {
	rexpr = builtin(env, sloc, lexpr, rexpr);

	return builtin_assign(env, sloc, lexpr, rexpr);
}
#define BUILTIN_ASSIGN(Name) \
static Ast \
builtin_assign_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_assign_op(env, sloc, lexpr, rexpr, builtin_##Name); \
}

BUILTIN_ASSIGN(land)
BUILTIN_ASSIGN(lor)
BUILTIN_ASSIGN(and)
BUILTIN_ASSIGN(or)
BUILTIN_ASSIGN(xor)
BUILTIN_ASSIGN(add)
BUILTIN_ASSIGN(sub)
BUILTIN_ASSIGN(mul)
BUILTIN_ASSIGN(div)
BUILTIN_ASSIGN(mod)
BUILTIN_ASSIGN(shl)
BUILTIN_ASSIGN(shr)
BUILTIN_ASSIGN(exl)
BUILTIN_ASSIGN(exr)
BUILTIN_ASSIGN(rol)
BUILTIN_ASSIGN(ror)

//------------------------------------------------------------------------------

static Ast
builtin_exchange_evaluate(
	Ast    env,
	sloc_t sloc,
	Ast    ast
) {
	ast = unquote(ast);

	if(ast_isArray(ast)) {
		Ast iexpr = eval(env, ast->m.rexpr);
		ast       = eval(env, ast->m.lexpr);

		switch(TYPE(ast_type(ast), ast_type(iexpr))) {
		case TYPE(AST_Environment, AST_Boolean):
		case TYPE(AST_Environment, AST_Integer):
		case TYPE(AST_Environment, AST_Character):
			if(ast_isAssignable(ast)) {
				size_t const index  = iexpr->m.ival;
				size_t const length = array_length(ast->m.env);
				if(index < length) {
					ast = array_at(ast->m.env, Ast, index);
					ast = deref(ast);
					if(ast_isAssignable(ast)) {
						return ast;
					}
				}
			}
			return oboerr(sloc, ERR_InvalidReferent);
		case TYPE(AST_Environment, AST_String):
			if(ast_isAssignable(ast)) {
				size_t const index  = atenv(ast, iexpr);
				size_t const length = array_length(ast->m.env);
				if(index < length) {
					ast = array_at(ast->m.env, Ast, index);
					ast = deref(ast);
					if(ast_isAssignable(ast)) {
						return ast;
					}
				}
			}
			return oboerr(sloc, ERR_InvalidReferent);
		default:
			return oboerr(sloc, ERR_InvalidReferent);
		}
	}

	if(ast_isnotZen(ast)) {
		ast = subeval(env, ast);

		if(ast_isReference(ast)) {
			ast = deref(ast);
			if(ast_isAssignable(ast)) {
				return ast;
			}
		}
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

static Ast
builtin_exchange(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	lexpr = builtin_exchange_evaluate(env, sloc, lexpr);
	if(ast_isError(lexpr)) {
		return lexpr;
	}
	rexpr = builtin_exchange_evaluate(env, sloc, rexpr);
	if(ast_isError(rexpr)) {
		return rexpr;
	}

	if(ast_isnotZen(lexpr) && ast_isnotZen(rexpr)) {
		if(lexpr != rexpr) {
			struct ast temp;

			memcpy(&temp, lexpr, sizeof(temp));
			memcpy(lexpr, rexpr, sizeof(temp));
			memcpy(rexpr, &temp, sizeof(temp));
		}

		return lexpr;
	}

	return oboerr(sloc, ERR_InvalidReferent);
}

//------------------------------------------------------------------------------

static Ast
builtin_range(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	return new_ast(sloc, AST_Operator, lexpr, rexpr, builtin_range_enum);
}

//------------------------------------------------------------------------------

static Ast
builtin_array(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	if(ast_isnotZen(lexpr)) {
		lexpr = eval(env, lexpr);
		rexpr = eval(env, rexpr);

		switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
		case TYPE(AST_Environment, AST_Boolean):
		case TYPE(AST_Environment, AST_Integer):
		case TYPE(AST_Environment, AST_Character): {
				size_t const index = rexpr->m.ival;
				if(index < array_length(lexpr->m.env)) {
					rexpr = array_at(lexpr->m.env, Ast, index);
					rexpr->attr |= lexpr->attr & ATTR_NoAssign;
					return rexpr;
				}
			}
			return oboerr(sloc, ERR_InvalidOperand);
		case TYPE(AST_Environment, AST_String):
			rexpr = inenv(lexpr, rexpr);
			rexpr->attr |= lexpr->attr & ATTR_NoAssign;
			return rexpr;
		case TYPE(AST_String, AST_Boolean):
		case TYPE(AST_String, AST_Integer):
		case TYPE(AST_String, AST_Character): {
				char const *cs = StringToCharLiteral(lexpr->m.sval, NULL);
				utf8off(cs, &cs, rexpr->m.ival);
				int  const  c = utf8chr(cs, NULL);
				if(~c) {
					return new_ast(rexpr->sloc, AST_Character, c);
				}
			}
			return oboerr(sloc, ERR_InvalidOperand);
		default:
			if(ast_isRange(rexpr)) switch(ast_type(lexpr)) {
			case AST_Environment: {
				size_t length = array_length(lexpr->m.env);
				size_t start  = ast_toInteger(eval(env, rexpr->m.lexpr));
				Ast    ast    = eval(env, rexpr->m.rexpr);
				size_t end    = ast_isZen(ast) ? length - !!length : ast_toInteger(ast);
				if(start > end) {
					size_t temp = start;
					start       = end;
					end         = temp;
				}
				if((start < length) && (end < length)) {
					length = end - start + 1;

					Array  arr = arrintop_alloc();
					arrintop_resize(arr, length);
					arrintop_copy  (arr, 0, lexpr->m.env, start, length);

					return new_ast(sloc, AST_Environment, arr, NULL);
				}
				break;
			}
			case AST_String: {
				size_t      length;
				char const *cs    = StringToCharLiteral(lexpr->m.sval, &length);
				length            = utf8len(cs, NULL, length);
				size_t      start = ast_toInteger(eval(env, rexpr->m.lexpr));
				Ast         ast   = eval(env, rexpr->m.rexpr);
				size_t      end   = ast_isZen(ast) ? length - !!length : ast_toInteger(ast);
				if(start > end) {
					size_t temp = start;
					start       = end;
					end         = temp;
				}
				if((start < length) && (end < length)) {
					start = utf8off(cs, NULL, start);
					end   = utf8off(cs, NULL, end + 1);

					String s = (length > 0) ? (
						SubString(lexpr->m.sval, start, end - start)
					):(
						StringCreate()
					);

					return new_ast(sloc, AST_String, s);
				}
				break;
			}
			default:
				break;
			}

			return oboerr(sloc, ERR_InvalidOperand);
		}
	}

	lexpr = new_env(sloc, NULL);

	if(ast_isAssemblage(rexpr)) for(;
		ast_isAssemblage(rexpr);
		rexpr = rexpr->m.rexpr
	) {
		if(ast_isnotZen(rexpr->m.lexpr)) {
			builtin_array_push_back(env, sloc, lexpr, rexpr->m.lexpr, BY_Value);
		}
	}
	else for(;
		ast_isSequence(rexpr);
		rexpr = rexpr->m.rexpr
	) {
		if(ast_isnotZen(rexpr->m.lexpr)) {
			builtin_array_push_back(env, sloc, lexpr, rexpr->m.lexpr, BY_Value);
		}
	}

	if(ast_isnotZen(rexpr)) {
		builtin_array_push_back(env, sloc, lexpr, rexpr, BY_Value);
	}

	return lexpr;
}

//------------------------------------------------------------------------------

static Ast
builtin_applicate(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	if(ast_isApplicate(lexpr)) {
		Ast ast = eval(env, lexpr->m.lexpr);

		if(ast_isEnvironment(ast)) {
			env = link_env(sloc, ast, env);

			return builtin_applicate(env, sloc, lexpr->m.rexpr, rexpr);
		}

		lexpr = builtin_applicate(env, sloc, ast, lexpr->m.rexpr);

	} else {
		lexpr = eval(env, lexpr);
	}

	switch(ast_type(lexpr)) {
	case AST_Zen:
		(void)eval(env, rexpr);
		return ZEN;
	case AST_Boolean:
	case AST_Integer:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Character: {
				String s = RepeatedCharToString(rexpr->m.ival, lexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_String: {
				String s = RepeatedString(rexpr->m.sval, lexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_Function: {
				Array statics_env = statics->m.env;
				Ast   locals      = source_env(sloc_source(rexpr->sloc));
				statics->m.env    = locals->m.env;

				locals = new_env(sloc, env);
				addenv_args(locals, env, sloc, rexpr->m.lexpr, lexpr);
				rexpr = refeval(locals, rexpr->m.rexpr);

				statics->m.env = statics_env;
				return rexpr;
			}
		case AST_BuiltinFunction:
			return rexpr->m.bfn(env, sloc, lexpr);
		default:
			return builtin_mul(env, sloc, lexpr, rexpr);
		}
	case AST_Float:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Character: {
				String s = RepeatedCharToString(rexpr->m.ival, (uint64_t)lexpr->m.fval);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_String: {
				String s = RepeatedString(rexpr->m.sval, (uint64_t)lexpr->m.fval);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_Function: {
				Array statics_env = statics->m.env;
				Ast   locals      = source_env(sloc_source(rexpr->sloc));
				statics->m.env    = locals->m.env;

				locals = new_env(sloc, env);
				addenv_args(locals, env, sloc, rexpr->m.lexpr, lexpr);
				rexpr = eval(locals, rexpr->m.rexpr);

				statics->m.env = statics_env;
				return rexpr;
			}
		case AST_BuiltinFunction:
			return rexpr->m.bfn(env, sloc, lexpr);
		default:
			return builtin_mul(env, sloc, lexpr, rexpr);
		}
	case AST_Function: {
			Array statics_env = statics->m.env;
			Ast   locals      = source_env(sloc_source(lexpr->sloc));
			statics->m.env    = locals->m.env;

			locals = new_env(sloc, env);
			addenv_args(locals, env, sloc, lexpr->m.lexpr, rexpr);
			rexpr = eval(locals, lexpr->m.rexpr);

			statics->m.env = statics_env;
			return rexpr;
		}
	case AST_Environment:
		if(ast_isIdentifier(rexpr) || ast_isString(rexpr)) {
			rexpr = inenv(lexpr, rexpr);
			if(ast_isnotZen(rexpr)) {
				rexpr->attr |= lexpr->attr & ATTR_NoAssign;
				return rexpr;
			}
			return oboerr(sloc, ERR_InvalidIdentifier);
		}
		env = link_env(sloc, lexpr, env);
		return refeval(env, rexpr);
	case AST_BuiltinFunction:
		return lexpr->m.bfn(env, sloc, rexpr);
	case AST_Character:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Boolean:
		case AST_Integer: {
				String s = RepeatedCharToString(lexpr->m.ival, rexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_Float: {
				String s = RepeatedCharToString(lexpr->m.ival, (uint64_t)rexpr->m.fval);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_Character: {
				String s = CharsToString(lexpr->m.ival, rexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		case AST_String: {
				String s = StringConcatenate(CharToString(lexpr->m.ival), rexpr->m.sval);
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
		default:
			break;
		}
		break;
	case AST_String:
		if(ast_isnotArray(rexpr)) {
			rexpr = eval(env, rexpr);
			switch(ast_type(rexpr)) {
			case AST_Boolean:
			case AST_Integer: {
					String s = RepeatedString(lexpr->m.sval, rexpr->m.ival);
					assert(s != NULL);
					return new_ast(sloc, AST_String, s);
				}
			case AST_Float: {
					String s = RepeatedString(lexpr->m.sval, (uint64_t)rexpr->m.fval);
					assert(s != NULL);
					return new_ast(sloc, AST_String, s);
				}
			case AST_Character: {
				String s = StringConcatenate(lexpr->m.sval, CharToString(rexpr->m.ival));
				assert(s != NULL);
				return new_ast(sloc, AST_String, s);
			}
			case AST_String: {
					String s = StringConcatenate(lexpr->m.sval, rexpr->m.sval);
					assert(s != NULL);
					return new_ast(sloc, AST_String, s);
				}
			default:
				break;
			}
		}
		nobreak;
	default:
		if(ast_isOperator(rexpr)
			&& ast_isZen(rexpr->m.lexpr)
		) {
			return evalop(env, sloc, rexpr->qual, lexpr, rexpr->m.rexpr);
		}
		break;
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

int
initialise_builtinop(
	Ast                    env,
	struct builtinop const builtinop[],
	size_t                 n_builtinop
) {
	size_t ts = gc_topof_stack();

	for(size_t i = 0; i < n_builtinop; ++i) {
		char const *cs    = builtinop[i].leme;
		size_t      n     = strlen(cs);
		uint64_t    hash  = memhash(cs, n, 0);
		String      s     = CharLiteralToString(cs, n);
		Ast         oper  = new_ast(0, AST_BuiltinOperator, s, builtinop[i].func, builtinop[i].prec);
		size_t      index = define(env, hash, oper, ATTR_NoAssign);
		assert(~index != 0);
		*builtinop[i].enup = index;
	}

	gc_revert(ts);

	return EXIT_SUCCESS;
}

int
initialise_builtinalias(
	Ast                       env,
	struct builtinalias const builtinalias[],
	size_t                    n_builtinalias
) {
	size_t ts = gc_topof_stack();

	for(size_t i = 0; i < n_builtinalias; ++i) {
		char const *cs   = builtinalias[i].alias;
		size_t      n    = strlen(cs);
		uint64_t    hash = memhash(cs, n, 0);
		String      s    = CharLiteralToString(cs, n);
		char const *ct   = builtinalias[i].op;
		String      t    = CharLiteralToString(ct, strlen(ct));
		Ast         ast  = new_ast(0, AST_String, t);

		operator_alias(env, 0, s, hash, ast);
	}

	gc_revert(ts);

	return EXIT_SUCCESS;
}

int
initialise_builtinfn(
	Ast                    env,
	struct builtinfn const builtinfn[],
	size_t                 n_builtinfn
) {
	size_t ts = gc_topof_stack();

	for(size_t i = 0; i < n_builtinfn; ++i) {
		char const *cs    = builtinfn[i].leme;
		size_t      n     = strlen(cs);
		uint64_t    hash  = memhash(cs, n, 0);
		String      s     = CharLiteralToString(cs, n);
		Ast         oper  = new_ast(0, AST_BuiltinFunction, s, builtinfn[i].func);
		size_t      index = define(env, hash, oper, ATTR_NoAssign);
		assert(~index != 0);
		*builtinfn[i].enup = index;
	}

	gc_revert(ts);

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static int
initialise_builtin_operators(
	void
) {
	static struct builtinop const builtinop[] = {
		BUILTIN("\\applicate"      , applicate      , P_Binding)
		BUILTIN("\\tag"            , tag            , P_Declarative)
		BUILTIN("\\tag_ref"        , tag_ref        , P_Declarative)
		BUILTIN("\\const"          , const          , P_Declarative)
		BUILTIN("\\global_tag"     , global_tag     , P_Declarative)
		BUILTIN("\\global_tag_ref" , global_tag_ref , P_Declarative)
		BUILTIN("\\global_const"   , global_const   , P_Declarative)
		BUILTIN("\\assign"         , assign         , P_Assigning)
		BUILTIN("\\assign_ref"     , assign_ref     , P_Assigning)
		BUILTIN("\\assign_land"    , assign_land    , P_Assigning)
		BUILTIN("\\assign_lor"     , assign_lor     , P_Assigning)
		BUILTIN("\\assign_and"     , assign_and     , P_Assigning)
		BUILTIN("\\assign_or"      , assign_or      , P_Assigning)
		BUILTIN("\\assign_xor"     , assign_xor     , P_Assigning)
		BUILTIN("\\assign_add"     , assign_add     , P_Assigning)
		BUILTIN("\\assign_sub"     , assign_sub     , P_Assigning)
		BUILTIN("\\assign_mul"     , assign_mul     , P_Assigning)
		BUILTIN("\\assign_div"     , assign_div     , P_Assigning)
		BUILTIN("\\assign_mod"     , assign_mod     , P_Assigning)
		BUILTIN("\\assign_shl"     , assign_shl     , P_Assigning)
		BUILTIN("\\assign_shr"     , assign_shr     , P_Assigning)
		BUILTIN("\\assign_exl"     , assign_exl     , P_Assigning)
		BUILTIN("\\assign_exr"     , assign_exr     , P_Assigning)
		BUILTIN("\\assign_rol"     , assign_rol     , P_Assigning)
		BUILTIN("\\assign_ror"     , assign_ror     , P_Assigning)
		BUILTIN("\\exchange"       , exchange       , P_Assigning)
		BUILTIN("\\if"             , if             , P_Conditional)
		BUILTIN("\\ifnot"          , ifnot          , P_Conditional)
		BUILTIN("\\case"           , case           , P_Conditional)
		BUILTIN("\\while"          , while          , P_Conditional)
		BUILTIN("\\until"          , until          , P_Conditional)
		BUILTIN("\\land"           , land           , P_Logical)
		BUILTIN("\\lor"            , lor            , P_Logical)
		BUILTIN("\\lt"             , lt             , P_Relational)
		BUILTIN("\\lte"            , lte            , P_Relational)
		BUILTIN("\\eq"             , eq             , P_Relational)
		BUILTIN("\\neq"            , neq            , P_Relational)
		BUILTIN("\\gte"            , gte            , P_Relational)
		BUILTIN("\\gt"             , gt             , P_Relational)
		BUILTIN("\\and"            , and            , P_Bitwise)
		BUILTIN("\\or"             , or             , P_Bitwise)
		BUILTIN("\\xor"            , xor            , P_Bitwise)
		BUILTIN("\\add"            , add            , P_Additive)
		BUILTIN("\\sub"            , sub            , P_Additive)
		BUILTIN("\\mul"            , mul            , P_Multiplicative)
		BUILTIN("\\div"            , div            , P_Multiplicative)
		BUILTIN("\\mod"            , mod            , P_Multiplicative)
		BUILTIN("\\shl"            , shl            , P_Exponential)
		BUILTIN("\\shr"            , shr            , P_Exponential)
		BUILTIN("\\exl"            , exl            , P_Exponential)
		BUILTIN("\\exr"            , exr            , P_Exponential)
		BUILTIN("\\rol"            , rol            , P_Exponential)
		BUILTIN("\\ror"            , ror            , P_Exponential)
		BUILTIN("\\array"          , array          , P_Binding)
		BUILTIN("\\range"          , range          , P_Binding)
	};
	static size_t const n_builtinop = sizeof(builtinop) / sizeof(builtinop[0]);

	static struct builtinalias const builtinalias[] = {
		{    "", "\\applicate"      },
		{   ":", "\\tag"            },
		{  ":^", "\\tag_ref"        },
		{  "::", "\\const"          },
		{ "(:)", "\\global_tag"     },
		{"(:^)", "\\global_tag_ref" },
		{"(::)", "\\global_const"   },
		{   "=", "\\assign"         },
		{  "=^", "\\assign_ref"     },
		{ "&&=", "\\assign_land"    },
		{ "||=", "\\assign_lor"     },
		{  "&=", "\\assign_and"     },
		{  "|=", "\\assign_or"      },
		{  "~=", "\\assign_xor"     },
		{  "+=", "\\assign_add"     },
		{  "-=", "\\assign_sub"     },
		{  "*=", "\\assign_mul"     },
		{  "/=", "\\assign_div"     },
		{ "//=", "\\assign_mod"     },
		{ "<<=", "\\assign_shl"     },
		{ ">>=", "\\assign_shr"     },
		{"<<<=", "\\assign_exl"     },
		{">>>=", "\\assign_exr"     },
		{"<<>=", "\\assign_rol"     },
		{"<>>=", "\\assign_ror"     },
		{  "><", "\\exchange"       },
		{   "?", "\\if"             },
		{   "!", "\\ifnot"          },
		{  "?:", "\\case"           },
		{  "?*", "\\while"          },
		{  "!*", "\\until"          },
		{  "&&", "\\land"           },
		{  "||", "\\lor"            },
		{   "<", "\\lt"             },
		{  "<=", "\\lte"            },
		{  "==", "\\eq"             },
		{  "<>", "\\neq"            },
		{  ">=", "\\gte"            },
		{   ">", "\\gt"             },
		{   "&", "\\and"            },
		{   "|", "\\or"             },
		{   "~", "\\xor"            },
		{   "+", "\\add"            },
		{   "-", "\\sub"            },
		{   "*", "\\mul"            },
		{   "/", "\\div"            },
		{  "//", "\\mod"            },
		{  "<<", "\\shl"            },
		{  ">>", "\\shr"            },
		{ "<<<", "\\exl"            },
		{ ">>>", "\\exr"            },
		{ "<<>", "\\rol"            },
		{ "<>>", "\\ror"            },
		{  "[]", "\\array"          },
		{  "..", "\\range"          },
	};
	static size_t const n_builtinalias = sizeof(builtinalias) / sizeof(builtinalias[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		initialise_builtinop   (operators, builtinop   , n_builtinop);
		initialise_builtinalias(operators, builtinalias, n_builtinalias);
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

int
initialise_builtin_math_functions(
	Ast env
) {
	static struct builtinfn const builtinfn[] = {
#		undef  DOUBLE_MATH1_FN
#		define DOUBLE_MATH1_FN(Name)  BUILTIN(#Name, Name)
#		undef  DOUBLE_MATH2_FN
#		define DOUBLE_MATH2_FN(Name)  BUILTIN(#Name, Name)

		DOUBLE_BUILTINS
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		return initialise_builtinfn(env, builtinfn, n_builtinfn);
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static int
initialise_builtin_datatypes(
	void
) {
	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

int
initialise_builtins(
	bool has_math
) {
	initialise_builtin_datatypes();
	initialise_builtin_operators();
	if(has_math) initialise_builtin_math_functions(globals);

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static String
mapoboefilewithpath(
	StringConst path,
	StringConst file,
	bool        ext
) {
	String filepath = StringConcatenate(path, file);
	assert(filepath != NULL);

	size_t      len;
	char const *name = StringToCharLiteral(filepath, &len);
	String      s    = mapfile(name);
	if(!s && ext) {

		String filepath_ext = StringAppendCharLiteral(filepath, ".oboe", 5);
		assert(filepath_ext != NULL);
		name = StringToCharLiteral(filepath_ext, &len);
		s    = mapfile(name);

		if(filepath_ext != filepath) {
			StringDelete(filepath_ext);
		}
	}

	StringDelete(filepath);

	return s;
}

String
mapoboefile(
	StringConst file
) {
	String s = NULL;

	size_t      len;
	char const *name   = StringToCharLiteral(file, &len);
	bool const  nopath = is_absolutepath(name) || is_relativepath(name);
	char const *ext    = strrchr(name, '.');
	if(ext && (strcmp(".oboe", ext) != 0)) {
		ext = NULL;
	}

	size_t index = 0;
	for(StringConst path = NullString();;) {
		s = mapoboefilewithpath(path, file, !ext);
		if(s) break;

		if(nopath || (index >= num_searchpaths())) {
			break;
		}

		path = get_searchpath(index);
		if(!path) {
			break;
		}
		index++;
	}

	return s;
}

