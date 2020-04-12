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
#include "mapfile.h"
#include "strlib.h"
#include "assert.h"
#include "parse.h"
#include "tostr.h"
#include "hash.h"
#include "eval.h"
#include "env.h"
#include "odt.h"
#include "gc.h"
#include <stdlib.h>
#include <math.h>

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

static unsigned builtin_applicate_enum = -1;
static unsigned builtin_if_enum        = -1;
static unsigned builtin_ifnot_enum     = -1;
static unsigned builtin_case_enum      = -1;
static unsigned builtin_while_enum     = -1;
static unsigned builtin_until_enum     = -1;
static unsigned builtin_land_enum      = -1;
static unsigned builtin_lor_enum       = -1;
static unsigned builtin_lt_enum        = -1;
static unsigned builtin_lte_enum       = -1;
static unsigned builtin_eq_enum        = -1;
static unsigned builtin_neq_enum       = -1;
static unsigned builtin_gte_enum       = -1;
static unsigned builtin_gt_enum        = -1;
static unsigned builtin_and_enum       = -1;
static unsigned builtin_or_enum        = -1;
static unsigned builtin_xor_enum       = -1;
static unsigned builtin_add_enum       = -1;
static unsigned builtin_sub_enum       = -1;
static unsigned builtin_mul_enum       = -1;
static unsigned builtin_div_enum       = -1;
static unsigned builtin_mod_enum       = -1;
static unsigned builtin_shl_enum       = -1;
static unsigned builtin_shr_enum       = -1;
static unsigned builtin_exl_enum       = -1;
static unsigned builtin_exr_enum       = -1;
static unsigned builtin_rol_enum       = -1;
static unsigned builtin_ror_enum       = -1;
static unsigned builtin_tag_enum       = -1;
static unsigned builtin_assign_enum    = -1;
static unsigned builtin_array_enum     = -1;

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
ast_isArray(
	Ast ast
) {
	return ast_isOp(ast, builtin_array_enum);
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

typedef uint64_t (*LogicalOp)(bool, bool);
#define LOGICALOP(Name,...) \
	uint64_t \
	logical_##Name( \
		bool lval, \
		bool rval  \
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

typedef String (*StringOp)(String, String);
#define STRINGOP(Name,...) \
	String \
	string_##Name( \
		String lval, \
		String rval  \
	) { \
		__VA_ARGS__; \
	}

typedef String (*StrIntOp)(String, uint64_t);
#define STRINTOP(Name,...) \
	String \
	string_##Name( \
		String   lval, \
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

typedef uint64_t (*StringCmp)(String, String);
#define STRINGCMP(Name,...) \
	uint64_t \
	string_##Name( \
		String lval, \
		String rval  \
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

bool
ast_toBool(
	Ast ast
) {
	switch(ast_type(ast)) {
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
	case TYPE(AST_Integer, AST_Integer):
	case TYPE(AST_Integer, AST_Character):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return (lexpr->m.ival == rexpr->m.ival);
	case TYPE(AST_Integer, AST_Float):
		return float_equal((double)lexpr->m.ival, rexpr->m.fval);
	case TYPE(AST_Float, AST_Integer):
		return float_equal(lexpr->m.fval, (double)rexpr->m.ival);
	case TYPE(AST_Float, AST_Float):
		return float_equal(lexpr->m.fval, rexpr->m.fval);
	case TYPE(AST_String, AST_String):
		return StringEqual(lexpr->m.sval, rexpr->m.sval);
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return (lexpr->m.ival == 0);
	case TYPE(AST_Float, AST_Zen):
		return float_equal(lexpr->m.fval, 0.0);
	case TYPE(AST_String, AST_Zen):
		return StringEqual(lexpr->m.sval, NullString());
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

static Ast
undefer(
	Ast env,
	Ast ast
) {
	if(ast_isIdentifier(ast)) {
		ast = subeval(env, ast);
	}
	for(;
		ast_isDeferred(ast);
		ast = ast->m.rexpr
	);
	return ast;
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
		return new_ast(sloc, NULL, AST_Integer, cond);
	}

	env = new_env(sloc, env);

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

static Ast
builtin_case(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	env = new_env(sloc, env);

	for(lexpr = undefer(env, lexpr);
		ast_isAssemblage(lexpr);
		lexpr = lexpr->m.rexpr
	) {
		eval(env, lexpr->m.lexpr);
	}

	Ast sel, cond = eval(env, lexpr);

	for(rexpr = undefer(env, rexpr);
		ast_isAssemblage(rexpr);
		rexpr = rexpr->m.rexpr
	) {
		lexpr = rexpr->m.lexpr;

		if(ast_isTag(lexpr)) {
			sel = eval(env, lexpr->m.lexpr);
			if(are_equal(sel, cond)) {
				return refeval(env, lexpr->m.rexpr);
			}
			continue;
		}

		eval(env, lexpr);
	}

	if(ast_isTag(rexpr)) {
		sel = eval(env, rexpr->m.lexpr);
		if(are_equal(sel, cond)) {
			return refeval(env, rexpr->m.rexpr);
		}
		return ZEN;
	}

	return refeval(env, rexpr);
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

	env = new_env(sloc, env);

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
builtin_logical(
	Ast       env,
	sloc_t    sloc,
	Ast       lexpr,
	Ast       rexpr,
	LogicalOp logicalop
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	return new_ast(sloc, NULL, AST_Integer, logicalop(ast_toBool(lexpr), ast_toBool(rexpr)));
}
#define BUILTIN_LOGICAL(Name) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_logical(env, sloc, lexpr, rexpr, logical_##Name); \
}

static LOGICALOP(land, return lval && rval)
static LOGICALOP(lor , return lval || rval)

static BUILTIN_LOGICAL(land)
static BUILTIN_LOGICAL(lor )

//------------------------------------------------------------------------------

static Ast
builtin_compare(
	Ast        env,
	sloc_t     sloc,
	Ast        lexpr,
	Ast        rexpr,
	IntegerCmp integercmp,
	FloatCmp   floatcmp,
	StringCmp  stringcmp
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Integer, AST_Integer):
	case TYPE(AST_Integer, AST_Character):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integercmp(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Integer, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, floatcmp((double)lexpr->m.ival, rexpr->m.fval));
	case TYPE(AST_Float, AST_Integer):
		return new_ast(sloc, NULL, AST_Integer, floatcmp(lexpr->m.fval, (double)rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, floatcmp(lexpr->m.fval, rexpr->m.fval));
	case TYPE(AST_String, AST_String):
		return new_ast(sloc, NULL, AST_Integer, stringcmp(lexpr->m.sval, rexpr->m.sval));
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integercmp(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, floatcmp(lexpr->m.fval, 0.0));
	case TYPE(AST_String, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, stringcmp(lexpr->m.sval, NullString()));
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integercmp(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, floatcmp(0.0, rexpr->m.fval));
	case TYPE(AST_Zen, AST_String):
		return new_ast(sloc, NULL, AST_Integer, stringcmp(NullString(), rexpr->m.sval));
	case TYPE(AST_Error, AST_Error):
		return new_ast(sloc, NULL, AST_Integer, integercmp(lexpr->qual, rexpr->qual));
	default:
		switch(ast_type(lexpr)) {
		case AST_Zen: case AST_Integer: case AST_Character: case AST_Float: case AST_String: case AST_Error:
			switch(ast_type(rexpr)) {
			case AST_Zen: case AST_Integer: case AST_Character: case AST_Float: case AST_String: case AST_Error:
				return oboerr(sloc, ERR_InvalidOperand);
			default:
				return oboerr(rexpr->sloc, ERR_InvalidOperand);
			}
		default:
			return oboerr(lexpr->sloc, ERR_InvalidOperand);
		}
	}
}

#define BUILTIN_COMPARE(Name) \
Ast \
builtin_##Name( \
	Ast    env,   \
	sloc_t sloc,  \
	Ast    lexpr, \
	Ast    rexpr  \
) { \
	return builtin_compare(env, sloc, lexpr, rexpr, integer_##Name, float_##Name, string_##Name); \
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

static BUILTIN_COMPARE(lt)
static BUILTIN_COMPARE(lte)
static BUILTIN_COMPARE(eq)
static BUILTIN_COMPARE(neq)
static BUILTIN_COMPARE(gte)
static BUILTIN_COMPARE(gt)

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
	case TYPE(AST_Integer, AST_Integer):
	case TYPE(AST_Integer, AST_Character):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Integer, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, double_to_uint64_t(rexpr->m.fval)));
	case TYPE(AST_Float, AST_Integer):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), double_to_uint64_t(rexpr->m.fval)));
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), 0));
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(0, double_to_uint64_t(rexpr->m.fval)));
	default:
		switch(ast_type(lexpr)) {
		case AST_Error:
			return lexpr;
		case AST_Zen: case AST_Integer: case AST_Character: case AST_Float:
			switch(ast_type(rexpr)) {
			case AST_Error:
				return rexpr;
			case AST_Zen: case AST_Integer: case AST_Character: case AST_Float:
				return oboerr(sloc, ERR_InvalidOperand);
			default:
				return oboerr(rexpr->sloc, ERR_InvalidOperand);
			}
		default:
			return oboerr(lexpr->sloc, ERR_InvalidOperand);
		}
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
	case TYPE(AST_Integer, AST_Integer):
	case TYPE(AST_Integer, AST_Character):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Integer, AST_Float):
		return new_ast(sloc, NULL, AST_Float, floatop((double)lexpr->m.ival, rexpr->m.fval));
	case TYPE(AST_Float, AST_Integer):
		return new_ast(sloc, NULL, AST_Float, floatop(lexpr->m.fval, (double)rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, NULL, AST_Float, floatop(lexpr->m.fval, rexpr->m.fval));
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, NULL, AST_Float, floatop(lexpr->m.fval, 0.0));
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, NULL, AST_Float, floatop(0.0, rexpr->m.fval));
	default:
		switch(ast_type(lexpr)) {
		case AST_Error:
			return lexpr;
		case AST_Zen: case AST_Integer: case AST_Character: case AST_Float:
			switch(ast_type(rexpr)) {
			case AST_Error:
				return rexpr;
			case AST_Zen: case AST_Integer: case AST_Character: case AST_Float:
				return oboerr(sloc, ERR_InvalidOperand);
			default:
				return oboerr(rexpr->sloc, ERR_InvalidOperand);
			}
		default:
			return oboerr(lexpr->sloc, ERR_InvalidOperand);
		}
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
	StrIntOp  stringop
) {
	lexpr = eval(env, lexpr);
	rexpr = eval(env, rexpr);

	switch(TYPE(ast_type(lexpr), ast_type(rexpr))) {
	case TYPE(AST_Integer, AST_Integer):
	case TYPE(AST_Integer, AST_Character):
	case TYPE(AST_Character, AST_Integer):
	case TYPE(AST_Character, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, rexpr->m.ival));
	case TYPE(AST_Integer, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, (uint64_t)rexpr->m.fval));
	case TYPE(AST_Float, AST_Integer):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), rexpr->m.ival));
	case TYPE(AST_Float, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), (uint64_t)rexpr->m.fval));
	case TYPE(AST_String, AST_Integer):
		return new_ast(sloc, NULL, AST_String , stringop(lexpr->m.sval, rexpr->m.ival));
	case TYPE(AST_String, AST_Float):
		return new_ast(sloc, NULL, AST_String , stringop(lexpr->m.sval, (uint64_t)rexpr->m.fval));
	case TYPE(AST_Integer, AST_Zen):
	case TYPE(AST_Character, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integerop(lexpr->m.ival, 0));
	case TYPE(AST_Float, AST_Zen):
		return new_ast(sloc, NULL, AST_Integer, integerop(double_to_uint64_t(lexpr->m.fval), 0));
	case TYPE(AST_String, AST_Zen):
		return new_ast(sloc, NULL, AST_String , stringop(lexpr->m.sval, 0));
	case TYPE(AST_Zen, AST_Integer):
	case TYPE(AST_Zen, AST_Character):
		return new_ast(sloc, NULL, AST_Integer, integerop(0, rexpr->m.ival));
	case TYPE(AST_Zen, AST_Float):
		return new_ast(sloc, NULL, AST_Integer, integerop(0, (uint64_t)rexpr->m.fval));
	default:
		switch(ast_type(lexpr)) {
		case AST_Error:
			return lexpr;
		case AST_Zen: case AST_Integer: case AST_Float: case AST_String:
			switch(ast_type(rexpr)) {
			case AST_Error:
				return rexpr;
			case AST_Zen: case AST_Integer: case AST_Float:
				return oboerr(sloc, ERR_InvalidOperand);
			default:
				return oboerr(rexpr->sloc, ERR_InvalidOperand);
			}
		default:
			return oboerr(lexpr->sloc, ERR_InvalidOperand);
		}
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
	return builtin_bitmove(env, sloc, lexpr, rexpr, integer_##Name, string_##Name); \
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
	rval %= n;
	return CharLiteralToString(cs + rval, n - rval);
)
static STRINTOP (shr,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval %= n;
	return CharLiteralToString(cs, (n - rval));
)
static STRINTOP (exl,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval %= n;
	return CharLiteralToString(cs, rval);
)
static STRINTOP (exr,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval %= n;
	return CharLiteralToString(cs + (n - rval), rval);
)
static STRINTOP (rol,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval %= n;
	return CharLiteralsToString(cs + rval, n - rval, cs, rval);
)
static STRINTOP (ror,
	size_t      n;
	char const *cs = StringToCharLiteral(lval, &n);
	rval %= n;
	return CharLiteralsToString(cs + (n - rval), rval, cs, n - rval);
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
	case AST_Integer:
		return new_ast(sloc, NULL, AST_Float, floatfn((double)arg->m.ival));
	case AST_Float:
		return new_ast(sloc, NULL, AST_Float, floatfn(arg->m.fval));
	case AST_Zen:
		return oboerr(sloc, ERR_InvalidOperand);
	case AST_Error:
		return arg;
	default:
		return oboerr(arg->sloc, ERR_InvalidOperand);
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
		case TYPE(AST_Integer, AST_Integer):
			return new_ast(sloc, NULL, AST_Float, floatop((double)lexpr->m.ival, (double)rexpr->m.ival));
		case TYPE(AST_Integer, AST_Float):
			return new_ast(sloc, NULL, AST_Float, floatop((double)lexpr->m.ival, rexpr->m.fval));
		case TYPE(AST_Float, AST_Integer):
			return new_ast(sloc, NULL, AST_Float, floatop(lexpr->m.fval, (double)rexpr->m.ival));
		case TYPE(AST_Float, AST_Float):
			return new_ast(sloc, NULL, AST_Float, floatop(lexpr->m.fval, rexpr->m.fval));
		default: switch(ast_type(lexpr)) {
		case AST_Error:
			return lexpr;
		case AST_Zen: case AST_Integer: case AST_Float:
			switch(ast_type(rexpr)) {
			case AST_Error:
				return rexpr;
			case AST_Zen: case AST_Integer: case AST_Float:
				return oboerr(sloc, ERR_InvalidOperand);
			default:
				return oboerr(rexpr->sloc, ERR_InvalidOperand);
			}
		default:
			return oboerr(lexpr->sloc, ERR_InvalidOperand);
		}
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

static Ast
builtin_tag(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	lexpr = unquote(lexpr);

	if(ast_isApplicate(lexpr)) {
		if(ast_isIdentifier(lexpr->m.lexpr)
			&& ast_isParameters(lexpr->m.rexpr)
		) {
			rexpr = new_ast(sloc, NULL, AST_Function, lexpr->m.rexpr, rexpr);
			addenv(env, sloc, lexpr->m.lexpr, rexpr);
			return rexpr;
		}
		if(ast_isString(lexpr->m.lexpr) // for OperatorFunction
			&& ast_isParameters(lexpr->m.rexpr)
		) {
			String      s     = lexpr->m.lexpr->m.sval;
			uint64_t    hash  = lexpr->m.lexpr->m.hash;
			rexpr             = new_ast(sloc, NULL, AST_Function, lexpr->m.rexpr, rexpr);
			rexpr             = new_ast(sloc, NULL, AST_OperatorFunction, s, rexpr);
			size_t      index = define(operators, hash, rexpr);
			assert(~index != 0);
			return rexpr;
		}
		return oboerr(sloc, ERR_InvalidOperand);
	}

	switch(ast_type(lexpr)) {
	case AST_Zen:
		return ast_isnotQuoted(rexpr) ? (
			new_ast(sloc, NULL, AST_Quoted, rexpr)
		) : (
			rexpr
		);
	case AST_Identifier:
		rexpr = refeval(env, rexpr);
		addenv(env, sloc, lexpr, rexpr);
		return rexpr;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}
}

//------------------------------------------------------------------------------

static Ast
builtin_array_push_back(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	if(ast_isTag(rexpr)) {
		if(ast_isString(rexpr->m.lexpr) || ast_isIdentifier(rexpr->m.lexpr)) {
			Ast def = refeval(env, rexpr->m.rexpr);
			addenv(lexpr, sloc, rexpr->m.lexpr, def);
			return lexpr;
		}
	} else {
		rexpr = refeval(env, rexpr);
		bool appended = array_push_back(lexpr->m.env, Ast, rexpr);
		assert(appended);
		return lexpr;
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_assign(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	lexpr = unquote(lexpr);

	if(ast_isArray(lexpr)) {
		Ast iexpr = eval(env, lexpr->m.rexpr);
		lexpr     = eval(env, lexpr->m.lexpr);
		rexpr     = refeval(env, rexpr);

		switch(TYPE(ast_type(lexpr), ast_type(iexpr))) {
		case TYPE(AST_Environment, AST_Integer): {
				size_t const length = array_length(lexpr->m.env);
				size_t const index  = iexpr->m.ival;
				if(index < length) {
					array_at(lexpr->m.env, Ast, index) = rexpr;
					return rexpr;
				}
				if(index == length) {
					return builtin_array_push_back(env, sloc, lexpr, rexpr);
				}
			}
			return oboerr(sloc, ERR_InvalidOperand);
		case TYPE(AST_Environment, AST_String): {
				Ast def = inenv(lexpr, iexpr);
				if(ast_isnotZen(def)) {
					def->m.rexpr = rexpr;
				} else {
					rexpr = addenv(lexpr, sloc, iexpr, rexpr);
					if(ast_isnotZen(rexpr)) {
						rexpr = rexpr->m.rexpr;
					}
				}
				return rexpr;
			}
		default:
			return oboerr(sloc, ERR_InvalidReferent);
		}
	}

	if(ast_isnotZen(lexpr)) {
		lexpr = subeval(env, lexpr);
		rexpr = refeval(env, rexpr);

		if(ast_isReference(lexpr)) {
			for(lexpr = lexpr->m.rexpr;
				ast_isReference(lexpr);
				lexpr = lexpr->m.rexpr
			);
			if(ast_isnotZen(lexpr)) {
				return assign(sloc, lexpr, rexpr);
			}
		}

		return oboerr(sloc, ERR_InvalidReferent);
	}

	rexpr = eval(env, rexpr);
	lexpr = dup_ast(sloc, rexpr);
	return lexpr;
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
		case TYPE(AST_Environment, AST_Integer): {
				size_t const index = rexpr->m.ival;
				if(index < array_length(lexpr->m.env)) {
					return array_at(lexpr->m.env, Ast, index);
				}
			}
			return oboerr(sloc, ERR_InvalidOperand);
		case TYPE(AST_Environment, AST_String):
			rexpr = inenv(lexpr, rexpr);
			if(ast_isReference(rexpr)) {
				rexpr = rexpr->m.rexpr;
			}
			return rexpr;
		case TYPE(AST_String, AST_Integer): {
				size_t const index = rexpr->m.ival;
				int    const c     = StringGetChar(lexpr->m.sval, index);
				if(c >= 0) {
					return new_ast(rexpr->sloc, NULL, AST_Character, c);
				}
			}
			return oboerr(sloc, ERR_InvalidOperand);
		default:
			return oboerr(sloc, ERR_InvalidOperand);
		}
	}

	lexpr = new_env(sloc, NULL);

	for(;
		ast_isSequence(rexpr);
		rexpr = rexpr->m.rexpr
	) {
		if(ast_isnotZen(rexpr->m.lexpr)) {
			builtin_array_push_back(env, sloc, lexpr, rexpr->m.lexpr);
		}
	}

	if(ast_isnotZen(rexpr)) {
		builtin_array_push_back(env, sloc, lexpr, rexpr);
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
	lexpr = eval(env, lexpr);

	switch(ast_type(lexpr)) {
	case AST_Zen:
		(void)eval(env, rexpr);
		return ZEN;
	case AST_Integer:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Character: {
				String s = RepeatedCharToString(rexpr->m.ival, lexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
		case AST_String: {
				String s = RepeatedString(rexpr->m.sval, lexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
		case AST_Function: {
				Ast locals = new_env(sloc, env);
				addenv_args(locals, env, sloc, rexpr->m.lexpr, lexpr);
				return refeval(locals, rexpr->m.rexpr);
			}
		case AST_BuiltinFunction:
			return rexpr->m.bfn(env, sloc, lexpr);
		default:
			return builtin_mul(env, sloc, lexpr, rexpr);
		}
	case AST_Float:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Function: {
				Ast locals = new_env(sloc, env);
				addenv_args(locals, env, sloc, rexpr->m.lexpr, lexpr);
				return eval(locals, rexpr->m.rexpr);
			}
		case AST_BuiltinFunction:
			return rexpr->m.bfn(env, sloc, lexpr);
		default:
			return builtin_mul(env, sloc, lexpr, rexpr);
		}
	case AST_Function: {
			Ast locals = new_env(sloc, env);
			addenv_args(locals, env, sloc, lexpr->m.lexpr, rexpr);
			return eval(locals, lexpr->m.rexpr);
		}
	case AST_Environment:
		if(ast_isIdentifier(rexpr) || ast_isString(rexpr)) {
			lexpr = inenv(lexpr, rexpr);
			if(ast_isnotZen(lexpr)) {
				return lexpr;
			}
			return oboerr(sloc, ERR_InvalidIdentifier);
		}
		env = dup_env(sloc, lexpr, env);
		return refeval(env, rexpr);
	case AST_BuiltinFunction:
		return lexpr->m.bfn(env, sloc, rexpr);
	case AST_Character:
		rexpr = eval(env, rexpr);
		switch(ast_type(rexpr)) {
		case AST_Integer: {
				String s = RepeatedCharToString(lexpr->m.ival, rexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
		case AST_Character: {
				String s = CharsToString(lexpr->m.ival, rexpr->m.ival);
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
		case AST_String: {
				String s = StringConcatenate(CharToString(lexpr->m.ival), rexpr->m.sval);
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
		default:
			break;
		}
		break;
	case AST_String:
		if(!ast_isArray(rexpr)) {
			rexpr = eval(env, rexpr);
			switch(ast_type(rexpr)) {
			case AST_Integer: {
					String s = RepeatedString(lexpr->m.sval, rexpr->m.ival);
					assert(s != NULL);
					return new_ast(sloc, NULL, AST_String, s);
				}
			case AST_Character: {
				String s = StringConcatenate(lexpr->m.sval, CharToString(rexpr->m.ival));
				assert(s != NULL);
				return new_ast(sloc, NULL, AST_String, s);
			}
			case AST_String: {
					String s = StringConcatenate(lexpr->m.sval, rexpr->m.sval);
					assert(s != NULL);
					return new_ast(sloc, NULL, AST_String, s);
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
		Ast         oper  = new_ast(0, NULL, AST_BuiltinOperator, s, builtinop[i].func, builtinop[i].prec);
		size_t      index = define(env, hash, oper);
		assert(~index != 0);
		*builtinop[i].enup = index;
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
		Ast         oper  = new_ast(0, NULL, AST_BuiltinFunction, s, builtinfn[i].func);
		size_t      index = define(env, hash, oper);
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
		BUILTIN(   "", applicate, P_Binding)
		BUILTIN(  "=", assign   , P_Assigning)
		BUILTIN(  ":", tag      , P_Declarative)
		BUILTIN(  "?", if       , P_Conditional)
		BUILTIN(  "!", ifnot    , P_Conditional)
		BUILTIN( "?:", case     , P_Conditional)
		BUILTIN( "?*", while    , P_Conditional)
		BUILTIN( "!*", until    , P_Conditional)
		BUILTIN( "&&", land     , P_Logical)
		BUILTIN( "||", lor      , P_Logical)
		BUILTIN(  "<", lt       , P_Relational)
		BUILTIN( "<=", lte      , P_Relational)
		BUILTIN( "==", eq       , P_Relational)
		BUILTIN( "<>", neq      , P_Relational)
		BUILTIN( ">=", gte      , P_Relational)
		BUILTIN(  ">", gt       , P_Relational)
		BUILTIN(  "&", and      , P_Bitwise)
		BUILTIN(  "|", or       , P_Bitwise)
		BUILTIN(  "^", xor      , P_Bitwise)
		BUILTIN(  "+", add      , P_Additive)
		BUILTIN(  "-", sub      , P_Additive)
		BUILTIN(  "*", mul      , P_Multiplicative)
		BUILTIN(  "/", div      , P_Multiplicative)
		BUILTIN( "//", mod      , P_Multiplicative)
		BUILTIN( "<<", shl      , P_Exponential)
		BUILTIN( ">>", shr      , P_Exponential)
		BUILTIN("<<<", exl      , P_Exponential)
		BUILTIN(">>>", exr      , P_Exponential)
		BUILTIN("<<>", rol      , P_Exponential)
		BUILTIN("<>>", ror      , P_Exponential)
		BUILTIN( "[]", array    , P_Binding)
	};
	static size_t const n_builtinop = sizeof(builtinop) / sizeof(builtinop[0]);

	return initialise_builtinop(operators, builtinop, n_builtinop);
}

static int
initialise_builtin_math_functions(
	void
) {
	static struct builtinfn const builtinfn[] = {
#		undef  DOUBLE_MATH1_FN
#		define DOUBLE_MATH1_FN(Name)  BUILTIN(#Name, Name)
#		undef  DOUBLE_MATH2_FN
#		define DOUBLE_MATH2_FN(Name)  BUILTIN(#Name, Name)

		DOUBLE_BUILTINS
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	return initialise_builtinfn(globals, builtinfn, n_builtinfn);
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
	if(has_math) initialise_builtin_math_functions();

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

String
mapoboefile(
	String file
) {
	size_t      len;
	char const *name = StringToCharLiteral(file, &len);
	char const *ext  = strrchr(name, '.');
	if(ext && (strcmp(".oboe", ext) != 0)) {
		ext = NULL;
	}
	String s = mapfile(name);

	if(!s && !ext) {
		file = CharLiteralsToString(name, len, ".oboe", 5);
		assert(file != NULL);

		name = StringToCharLiteral(file, &len);
		s = mapfile(name);

		StringDelete(file);
	}

	return s;
}

