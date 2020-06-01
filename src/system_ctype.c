/*
MIT License

Copyright (c) 2020 Tristan Styles

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
#include "sources.h"
#include "system.h"
#include "strlib.h"
#include "assert.h"
#include "parse.h"
#include "tostr.h"
#include "eval.h"
#include "env.h"
#include "odt.h"
#include "gc.h"
#include "lex.h"
#include "utf8.h"
#include "nobreak.h"
#include <stdlib.h>
#include <stdarg.h>

//------------------------------------------------------------------------------

#define ENUM(Name,...)  static unsigned builtin_##Name##_enum = -1;
	ENUM(is_CharInSet)
	ENUM(to_Uppercase)
	ENUM(to_Lowercase)
#include "system_ctype.enum"

//------------------------------------------------------------------------------

typedef int (*is_CType)(char32_t);

static Ast
builtin_is_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype
) {
	arg = eval(env, arg);

	switch(arg->type) {
	case AST_Zen:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Integer, (uint64_t)is_ctype((char32_t)arg->m.ival));
	case AST_String: {
		int         res = 1;
		size_t      len;
		char const *cs = StringToCharLiteral(arg->m.sval, &len);
		do {
			char32_t c = utf8chr(cs, &cs);
			res = is_ctype(c);
		} while(res && *cs)
			;
		return new_ast(sloc, AST_Integer, (uint64_t)res);
	}
	default:
		switch(ast_type(arg)) {
		case AST_Error:
			return arg;
		default:
			return oboerr(sloc, ERR_InvalidOperand);
		}
	}
}

#define ENUM(Name,...) \
static Ast \
builtin_##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_is_ctype(env, sloc, arg, Name); \
}
#include "system_ctype.enum"

static Ast
builtin_is_CharInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	if(ast_isSequence(args)) {
		Ast      ast = eval(env, args->m.lexpr);
		char32_t c   = (char32_t)ast_toInteger(ast);

		for(args = args->m.rexpr; ast_isSequence(args); args = args->m.rexpr) {

			ast = eval(env, args->m.lexpr);
			if(ast_isString(ast)) {
				char const *cs = StringToCharLiteral(ast->m.sval, NULL);
				if(utf8strchr(cs, c) != NULL) {
					return new_ast(sloc, AST_Integer, UINT64_C(1));
				}
			} else {
				if(c == ast_toInteger(ast)) {
					return new_ast(sloc, AST_Integer, UINT64_C(1));
				}
			}
		}

		ast = eval(env, args);
		if(ast_isString(ast)) {
			char const *cs = StringToCharLiteral(ast->m.sval, NULL);
			if(utf8strchr(cs, c) != NULL) {
				return new_ast(sloc, AST_Integer, UINT64_C(1));
			}
		} else {
			if(c == ast_toInteger(ast)) {
				return new_ast(sloc, AST_Integer, UINT64_C(1));
			}
		}

		return new_ast(sloc, AST_Integer, UINT64_C(0));
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

typedef char32_t (*to_CType)(char32_t);

static Ast
builtin_convert_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	to_CType  to_ctype
) {
	arg = eval(env, arg);

	switch(arg->type) {
	case AST_Zen:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Character, (int)to_ctype((char32_t)arg->m.ival));
	case AST_String: {
		String s = StringCreate();
		assert(s != NULL);
		size_t      len;
		char const *cs = StringToCharLiteral(arg->m.sval, &len);
		if(len > 0) do {
			char     uc[4];
			char32_t c = utf8chr(cs, &cs);
			len = utf8encode(uc, NULL, to_ctype(c));
			if(len) {
				String t = StringAppendCharLiteral(s, uc, len);
				assert(t != NULL);
				s = t;
			}
		} while(*cs)
			;
		return new_ast(sloc, AST_String, s);
	}
	default:
		switch(ast_type(arg)) {
		case AST_Error:
			return arg;
		default:
			return oboerr(sloc, ERR_InvalidOperand);
		}
	}
}

static Ast
builtin_to_Uppercase(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_convert_ctype(env, sloc, arg, to_Uppercase);
}

static Ast
builtin_to_Lowercase(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_convert_ctype(env, sloc, arg, to_Lowercase);
}

//------------------------------------------------------------------------------

int
initialise_system_ctype(
	void
) {
	static struct builtinfn const builtinfn[] = {
#	define ENUM(Name,...) BUILTIN(#Name, Name)
		ENUM(is_CharInSet)
		ENUM(to_Uppercase)
		ENUM(to_Lowercase)
#	include "system_ctype.enum"
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	}

	return EXIT_SUCCESS;
}

