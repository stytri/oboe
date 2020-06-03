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
#include "system.h"
#include "assert.h"
#include "eval.h"
#include "bits.h"


//------------------------------------------------------------------------------

static unsigned builtin_popcount_enum = -1;
static unsigned builtin_bitmask_enum  = -1;
static unsigned builtin_lzcount_enum  = -1;
static unsigned builtin_msbit_enum    = -1;

//------------------------------------------------------------------------------

static inline uint64_t
double_to_uint64_t(
	double v
) {
	return ((union { double d; uint64_t u; }){ .d = v }).u;
}

//------------------------------------------------------------------------------

static Ast
builtin_popcount(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Integer, (uint64_t)popcount64(arg->m.ival));
	case AST_Float:
		return new_ast(sloc, AST_Integer, (uint64_t)popcount64(double_to_uint64_t(arg->m.fval)));
	case AST_Error:
		return arg;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}

}

static Ast
builtin_bitmask(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Integer, (uint64_t)bitmask64(arg->m.ival));
	case AST_Float:
		return new_ast(sloc, AST_Integer, (uint64_t)bitmask64(double_to_uint64_t(arg->m.fval)));
	case AST_Error:
		return arg;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}

}

static Ast
builtin_lzcount(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Integer, (uint64_t)lzcount64(arg->m.ival));
	case AST_Float:
		return new_ast(sloc, AST_Integer, (uint64_t)lzcount64(double_to_uint64_t(arg->m.fval)));
	case AST_Error:
		return arg;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}

}

static Ast
builtin_msbit(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Integer, (uint64_t)msbit64(arg->m.ival));
	case AST_Float:
		return new_ast(sloc, AST_Integer, (uint64_t)msbit64(double_to_uint64_t(arg->m.fval)));
	case AST_Error:
		return arg;
	default:
		return oboerr(sloc, ERR_InvalidOperand);
	}

}

//------------------------------------------------------------------------------

int
initialise_system_bits(
	void
) {
	static struct builtinfn const builtinfn[] = {
		BUILTIN("popcount", popcount)
		BUILTIN("bitmask" , bitmask)
		BUILTIN("lzcount" , lzcount)
		BUILTIN("msbit"   , msbit)
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		Ast var;
		var = new_ast(0, AST_Integer, (uint64_t)CHAR_BIT);
		addenv_named(system_environment, 0, "CHAR_BIT", var, ATTR_NoAssign);
		var = new_ast(0, AST_Integer, (uint64_t)SIZE_MAX);
		addenv_named(system_environment, 0, "SIZE_MAX", var, ATTR_NoAssign);

		initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	}

	return EXIT_SUCCESS;
}

