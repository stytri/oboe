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
#include "version.h"
#include "builtins.h"
#include "sources.h"
#include "system.h"
#include "strlib.h"
#include "assert.h"
#include "parse.h"
#include "tostr.h"
#include "hash.h"
#include "eval.h"
#include "env.h"
#include "gc.h"
#include <stdlib.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

//------------------------------------------------------------------------------

Ast system_environment = NULL;

//------------------------------------------------------------------------------

static unsigned builtin_sigil_enum         = -1;
static unsigned builtin_system_enum        = -1;
#define ENUM(Name,...) \
static unsigned builtin_is_##Name##_enum   = -1;
ENUM(Tag)
ENUM(Applicate)
ENUM(Array)
#include "oboe.enum"
static unsigned builtin_type_enum          = -1;
static unsigned builtin_typename_enum      = -1;
static unsigned builtin_length_enum        = -1;
static unsigned builtin_to_String_enum     = -1;
static unsigned builtin_to_Literal_enum    = -1;
static unsigned builtin_to_Integer_enum    = -1;
static unsigned builtin_to_Float_enum      = -1;
static unsigned builtin_assert_enum        = -1;
static unsigned builtin_getenv_enum        = -1;
static unsigned builtin_setlocale_enum     = -1;
static unsigned builtin_clock_enum         = -1;
static unsigned builtin_time_enum          = -1;
static unsigned builtin_difftime_enum      = -1;
static unsigned builtin_localtime_enum     = -1;
static unsigned builtin_utctime_enum       = -1;
static unsigned builtin_rand_enum          = -1;
static unsigned builtin_randf_enum         = -1;
static unsigned builtin_eval_enum          = -1;
static unsigned builtin_parse_enum         = -1;
static unsigned builtin_load_enum          = -1;
static unsigned builtin_import_enum        = -1;
static unsigned builtin_exit_enum          = -1;

//------------------------------------------------------------------------------

Ast
error_or(
	sloc_t sloc,
	Ast    ast,
	Error  err
) {
	return ast_isError(ast) ? (
		ast
	) : (
		oboerr(sloc, err)
	);
}

Ast
sequential_evaluation(
	Ast  (*evaluator)(
		Ast    env,
		sloc_t sloc,
		Ast    arg
	),
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	for(;
		ast_isSequence(arg);
		arg = arg->m.rexpr
	) {
		Ast ast = evaluator(env, sloc, arg->m.lexpr);
		if(ast_isError(ast)) {
			return ast;
		}
	}

	return evaluator(env, sloc, arg);
}

//------------------------------------------------------------------------------

static int
initialise_errors(
	void
) {
	static struct {
		char const *leme;
		Error       err;
	} builtinerr[] = {
#	define ENUM(Name,...)  { "ERROR_"#Name, ERR_##Name },
#	include "oboerr.enum"
	};
	static size_t const n_builtinerr = sizeof(builtinerr) / sizeof(builtinerr[0]);

	size_t ts = gc_topof_stack();

	for(size_t i = 0; i < n_builtinerr; ++i) {
		char const *cs    = builtinerr[i].leme;
		size_t      n     = strlen(cs);
		uint64_t    hash  = memhash(cs, n, 0);
		String      s     = CharLiteralToString(cs, n);
		Ast         err   = new_ast(0, NULL, AST_Error, builtinerr[i].err);
		Ast         def   = new_ast(0, NULL, AST_Reference, s, err);
		size_t      index = define(system_environment, hash, def);
		assert(~index != 0);
	}

	gc_revert(ts);

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static int
initialise_datatypes(
	void
) {
	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static Ast
builtin_sigil(
	Ast    env,
	sloc_t sloc,
	Ast    lexpr,
	Ast    rexpr
) {
	if(ast_isIdentifier(rexpr) || ast_isString(rexpr)) {
		for(rexpr = inenv(system_environment, rexpr);
			ast_isReference(rexpr);
			rexpr = rexpr->m.rexpr
		);

		if(ast_isBuiltinFunction(rexpr)) {

			if(ast_isnotZen(lexpr)) {
				return rexpr->m.bfn(env, sloc, lexpr);
			}
		}

		if(ast_isnotZen(rexpr)) {
			return rexpr;
		}
	}

	return oboerr(sloc, ERR_InvalidIdentifier);
}

//------------------------------------------------------------------------------

static Ast
builtin_system_1(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	if(ast_isString(arg)) {

		char const *cs = StringToCharLiteral(arg->m.sval, NULL);
		int         r  = system(cs);

		return new_ast(sloc, NULL, AST_Integer, (uint64_t)r);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_system(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return sequential_evaluation(builtin_system_1, env, sloc, arg);
}

//------------------------------------------------------------------------------

#define ENUM(Name,...) \
static Ast \
builtin_is_##Name( \
	Ast    env,  \
	sloc_t sloc, \
	Ast    arg   \
) { \
	arg = dereference(env, arg); \
	uint64_t is = ast_is##Name(arg); \
	return new_ast(sloc, NULL, AST_Integer, is); \
}
ENUM(Tag)
ENUM(Applicate)
ENUM(Array)
#define builtin_is_Identifier builtin_is_Identifier__hidden
#include "oboe.enum"
#undef  builtin_is_Identifier

static Ast
builtin_is_Identifier(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	uint64_t is = ast_isIdentifier(arg);
	return new_ast(sloc, NULL, AST_Integer, is);
	(void)env;
}

static Ast
builtin_type(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	uint64_t type = arg->type;
	return new_ast(sloc, NULL, AST_Integer, type);
}

static Ast
builtin_typename(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	char const *cs = ast_typename(arg);
	String      s  = CharLiteralToString(cs, strlen(cs));
	return new_ast(sloc, NULL, AST_String, s);
}

static Ast
builtin_length(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	size_t len = 0;
	switch(ast_type(arg)) {
	case AST_String: case AST_Identifier:
		len = StringLength(arg->m.sval);
		break;
	case AST_Sequence:
		for(len = 1; ast_isSequence(arg); arg = arg->m.rexpr) {
			++len;
		}
		break;
	case AST_Assemblage:
		for(len = 1; ast_isAssemblage(arg); arg = arg->m.rexpr) {
			++len;
		}
		break;
	case AST_Environment:
		len = array_length(arg->m.env);
		break;
	default:
		break;
	}
	return new_ast(sloc, NULL, AST_Integer, len);
}

//------------------------------------------------------------------------------

static Ast
builtin_to_String(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	String s = tostr(arg, false);
	assert(s != NULL);
	return new_ast(sloc, NULL, AST_String, s);
}

static Ast
builtin_to_Literal(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	String s = tostr(arg, true);
	assert(s != NULL);
	return new_ast(sloc, NULL, AST_String, s);
}

static Ast
builtin_to_Integer(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
		return new_ast(sloc, NULL, AST_Integer, (uint64_t)0);
	case AST_Integer:
		return arg;
	case AST_Character:
		return new_ast(sloc, NULL, AST_Integer, arg->m.ival);
	case AST_Float:
		return new_ast(sloc, NULL, AST_Integer, (uint64_t)arg->m.fval);
	case AST_String: {
			char const *cs   = StringToCharLiteral(arg->m.sval, NULL);
			uint64_t    ival = strtou(cs, NULL);
			return new_ast(sloc, NULL, AST_Integer, ival);
		}
	case AST_Error:
		return new_ast(sloc, NULL, AST_Integer, (uint64_t)arg->qual);
	default:
		break;
	}

	return ZEN;
}

static Ast
builtin_to_Float(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	switch(ast_type(arg)) {
	case AST_Zen:
		return new_ast(sloc, NULL, AST_Float, (double)0);
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, NULL, AST_Float, (double)arg->m.ival);
	case AST_Float:
		return arg;
	case AST_String: {
			char const *cs   = StringToCharLiteral(arg->m.sval, NULL);
			double      fval = strtod(cs, 0);
			return new_ast(sloc, NULL, AST_Float, fval);
		}
	case AST_Error:
		return new_ast(sloc, NULL, AST_Float, (double)arg->qual);
	default:
		break;
	}

	return ZEN;
}

//------------------------------------------------------------------------------

static Ast
builtin_assert(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	Ast  msg = ZEN;
	Ast  eof = ZEN;

	if(ast_isSequence(arg)) {
		msg = arg->m.rexpr;

		if(ast_isSequence(msg)) {
			eof = arg->m.rexpr;
			msg = msg->m.lexpr;
		}

		arg = arg->m.lexpr;
	}

	bool cond = ast_toBool(eval(env, arg));
	if(!cond) {

		msg = eval(env, msg);
		if(ast_isZen(msg)) {
			static char const assertion_failed[] = "!ASSERTION FAILED!";
			String s = CharLiteralToString(assertion_failed, sizeof(assertion_failed)-1);
			assert(s != NULL);
			msg = new_ast(sloc, NULL, AST_String, s);
		}

		String      s      = StringCreate();
		String      srcs   = get_source(sloc_source(sloc));
		char const *source = StringToCharLiteral(srcs, NULL);
		char        line[(CHAR_BIT * sizeof(unsigned)) + 1];
		snprintf(line, sizeof(line), ":%u: ", sloc_line(sloc));

		s = StringAppendCharLiteral(s, source , strlen(source));
		s = StringAppendCharLiteral(s, line   , strlen(line));
		s = StringAppend(s, tostr(msg, false));
		assert(s != NULL);

		fflush(stderr);
		fputs(StringToCharLiteral(s, NULL), stderr);
		fputc('\n', stderr);
		fflush(stderr);

		eof = eval(env, eof);
		switch(ast_type(eof)) {
		case AST_Integer:
			cond = eof->m.ival != 0;
			break;
		case AST_String:
			cond = StringEqualCharLiteral(eof->m.sval, "fatal", 5);
			break;
		default:
			break;
		}
		if(cond) {
			exit(EXIT_FAILURE);
		}
	}

	return ZEN;
}

//------------------------------------------------------------------------------

static Ast
builtin_getenv(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isnotSequence(arg)) {
		arg = eval(env, arg);
		if(ast_isString(arg)) {

			char const *cs = StringToCharLiteral(arg->m.sval, NULL);
			cs = getenv(cs);
			String s = CharLiteralToString(cs, strlen(cs));
			assert(s != NULL);

			return new_ast(sloc, NULL, AST_String, s);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_setlocale(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isSequence(arg)) {
		Ast ast = eval(env, arg->m.lexpr);
		arg     = eval(env, arg->m.rexpr);

		for(int loc = LC_ALL;
			ast_isString(ast) && ast_isString(arg);
		) {
			if(StringEqualCharLiteral(ast->m.sval, "collate", 7)) {
				loc = LC_COLLATE;
			} else if(StringEqualCharLiteral(ast->m.sval, "ctype", 5)) {
				loc = LC_CTYPE;
			} else if(StringEqualCharLiteral(ast->m.sval, "monetary", 8)) {
				loc = LC_MONETARY;
			} else if(StringEqualCharLiteral(ast->m.sval, "numeric", 7)) {
				loc = LC_NUMERIC;
			} else if(StringEqualCharLiteral(ast->m.sval, "time", 4)) {
				loc = LC_TIME;
			} else if(!StringEqualCharLiteral(ast->m.sval, "all", 3)) {
				break;
			}

			char const *cs = StringToCharLiteral(arg->m.sval, NULL);
			cs             = setlocale(loc, cs);
			if(!cs) {
				break;
			}

			String s = CharLiteralToString(cs, strlen(cs));
			assert(s != NULL);

			return new_ast(sloc, NULL, AST_String, s);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_clock(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	static_assert((sizeof(clock_t) <= sizeof(uint64_t)) && (sizeof(clock_t) <= sizeof(double)),
		"(sizeof(clock_t) <= sizeof(uint64_t)) && (sizeof(clock_t) <= sizeof(double))"
	);

	if(ast_isZen(arg)) {
		clock_t t = clock();

		if((clock_t)0.5 == 0) {
			return new_ast(sloc, NULL, AST_Integer, (uint64_t)t);

		} else {
			return new_ast(sloc, NULL, AST_Float, (double)t);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
}

static Ast
builtin_time(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	static_assert((sizeof(time_t) <= sizeof(uint64_t)) && (sizeof(time_t) <= sizeof(double)),
		"(sizeof(time_t) <= sizeof(uint64_t)) && (sizeof(time_t) <= sizeof(double))"
	);

	if(ast_isnotZen(arg)) {
		arg = eval(env, arg);
	}

	time_t t;
	bool   ok = true;

	if(ast_isZen(arg)) {
		t = time(NULL);

	} else if(ast_isEnvironment(arg)) {
		struct tm tm = { 0 };
		Ast       ast;
		ast = eval_named(arg, sloc, "seconds");
		ok  = ast_isInteger(ast);
		if(ok) {
			tm.tm_sec = ast->m.ival;

			ast = eval_named(arg, sloc, "minutes");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_min = ast->m.ival;

			ast = eval_named(arg, sloc, "hour");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_hour = ast->m.ival;

			ast = eval_named(arg, sloc, "day");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_mday = ast->m.ival;

			ast = eval_named(arg, sloc, "month");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_mon = ast->m.ival;

			ast = eval_named(arg, sloc, "year");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_year = ast->m.ival;

			ast = eval_named(arg, sloc, "is DST");
			ok  = ast_isInteger(ast);
		}
		if(ok) {
			tm.tm_isdst = ast->m.ival;

			t = mktime(&tm);
		}

	} else {
		ok = false;
	}

	if(ok) {
		if((time_t)0.5 == 0) {
			return new_ast(sloc, NULL, AST_Integer, (uint64_t)t);

		} else {
			return new_ast(sloc, NULL, AST_Float, (double)t);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
}

static Ast
builtin_difftime(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isSequence(arg)) {
		double dt;

		if((time_t)0.5 == 0) {
			Ast ast = builtin_to_Integer(env, sloc, arg->m.lexpr);
			arg     = builtin_to_Integer(env, sloc, arg->m.rexpr);

			if(ast_isInteger(ast) && ast_isInteger(arg)) {
				dt = difftime((time_t)ast->m.ival, (time_t)arg->m.ival);

				return new_ast(sloc, NULL, AST_Float, dt);
			}

		} else {
			Ast ast = builtin_to_Float(env, sloc, arg->m.lexpr);
			arg     = builtin_to_Float(env, sloc, arg->m.rexpr);

			if(ast_isFloat(ast) && ast_isFloat(arg)) {
				dt = difftime((time_t)ast->m.fval, (time_t)arg->m.fval);

				return new_ast(sloc, NULL, AST_Float, dt);
			}
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
}

static Ast
convert_time_to_env(
	sloc_t       sloc,
	struct tm *(*conv)(
		time_t const *tp
	),
	time_t       t
) {
	struct tm *tp = conv(&t);
	if(tp) {
		struct tm tm;
		memcpy(&tm, tp, sizeof(tm));
		Ast env = new_env(sloc, NULL), ast;

		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_sec);
		addenv_named(env, sloc, "seconds", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_min);
		addenv_named(env, sloc, "minutes", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_hour);
		addenv_named(env, sloc, "hour", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_mday);
		addenv_named(env, sloc, "day", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_mon);
		addenv_named(env, sloc, "month", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_year);
		addenv_named(env, sloc, "year", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_wday);
		addenv_named(env, sloc, "week day", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_yday);
		addenv_named(env, sloc, "year day", ast);
		ast = new_ast(sloc, NULL, AST_Integer, (uint64_t)tm.tm_isdst);
		addenv_named(env, sloc, "is DST", ast);

		return env;
	}

	return oboerr(sloc, ERR_FailedOperation);
}

static Ast
builtin_localtime(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if((time_t)0.5 == 0) {
		arg = builtin_to_Integer(env, sloc, arg);
		if(ast_isInteger(arg)) {
			return convert_time_to_env(sloc, localtime, (time_t)arg->m.ival);
		}

	} else {
		arg = builtin_to_Float(env, sloc, arg);
		if(ast_isFloat(arg)) {
			return convert_time_to_env(sloc, localtime, (time_t)arg->m.fval);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_utctime(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if((time_t)0.5 == 0) {
		arg = builtin_to_Integer(env, sloc, arg);
		if(ast_isInteger(arg)) {
			return convert_time_to_env(sloc, gmtime, (time_t)arg->m.ival);
		}

	} else {
		arg = builtin_to_Float(env, sloc, arg);
		if(ast_isFloat(arg)) {
			return convert_time_to_env(sloc, gmtime, (time_t)arg->m.fval);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static void
initialize_rand(
	void
) {
	int s = time(NULL);
	srand(s);
}

static Ast
builtin_rand(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	int const r = rand();
	return new_ast(sloc, NULL, AST_Integer, (uint64_t)r);

	(void)env;
	(void)sloc;
	(void)arg;
}

static Ast
builtin_randf(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	int const r = rand();
	return new_ast(sloc, NULL, AST_Float, (double)r / (double)RAND_MAX);

	(void)env;
	(void)sloc;
	(void)arg;
}

//------------------------------------------------------------------------------

static Ast
builtin_eval(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return eval(env, arg);
	(void)sloc;
}

static Ast
builtin_parse(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isnotSequence(arg)) {
		arg = eval(env, arg);
		if(ast_isString(arg)) {

			char const *args   = StringToCharLiteral(arg->m.sval, NULL);
			unsigned    source = sloc_source(sloc);
			unsigned    line   = sloc_line(sloc);

			arg = parse(args, &args, source, &line, new_ast, true);

			return new_ast(sloc, NULL, AST_Quoted, arg);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_load(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isnotSequence(arg)) {
		arg = eval(env, arg);
		if(ast_isString(arg)) {

			String s = mapoboefile(arg->m.sval);
			if(s) {
				return new_ast(sloc, NULL, AST_String, s);
			}
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_import_1(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	if(ast_isString(arg)) {

		String file = arg->m.sval;
		String s    = mapoboefile(file);
		if(s) {
			arg = ZEN;

			unsigned    source = add_source(0, file);
			unsigned    line   = 0;
			char const *cs     = StringToCharLiteral(s, NULL);

			for(size_t ts = gc_topof_stack();
				*cs;
			) {
				arg = parse(cs, &cs, source, &line, new_ast, false);
				if(ast_isnotZen(arg)) {
					arg = eval(globals, arg);
				}

				gc_return(ts, arg);
				run_gc();

				if(ast_isError(arg)) {
					break;
				}
			}

			StringDelete(s);

			return arg;
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_import(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return sequential_evaluation(builtin_import_1, env, sloc, arg);
}

//------------------------------------------------------------------------------

static Ast
builtin_exit(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isnotZen(arg)) {
		arg = eval(env, arg);
		switch(ast_type(arg)) {
		case AST_Zen:
			break;
		case AST_Integer:
			exit((int)arg->m.ival);
		default:
//			arg = builtin_print_line(env, sloc, arg);
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);

	(void)sloc;
}

//------------------------------------------------------------------------------

int
initialise_system_environment(
	void
) {
	static struct builtinop const builtinop[] = {
		BUILTIN("@", sigil, P_Binding)
	};
	static size_t const n_builtinop = sizeof(builtinop) / sizeof(builtinop[0]);

	static struct builtinfn const builtinfn[] = {
		BUILTIN("system"    , system)
#		define ENUM(Name,...)  \
		BUILTIN("is_"#Name   , is_##Name)
		ENUM(Tag)
		ENUM(Applicate)
		ENUM(Array)
#		include "oboe.enum"
		BUILTIN("type"      , type)
		BUILTIN("typename"  , typename)
		BUILTIN("length"    , length)
		BUILTIN("to_String" , to_String)
		BUILTIN("to_Literal", to_Literal)
		BUILTIN("to_Integer", to_Integer)
		BUILTIN("to_Float"  , to_Float)
		BUILTIN("assert"    , assert)
		BUILTIN("getenv"    , getenv)
		BUILTIN("setlocale" , setlocale)
		BUILTIN("clock"     , clock)
		BUILTIN("time"      , time)
		BUILTIN("difftime"  , difftime)
		BUILTIN("localtime" , localtime)
		BUILTIN("utctime"   , utctime)
		BUILTIN("rand"      , rand)
		BUILTIN("randf"     , randf)
		BUILTIN("eval"      , eval)
		BUILTIN("parse"     , parse)
		BUILTIN("load"      , load)
		BUILTIN("import"    , import)
		BUILTIN("exit"      , exit)
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	system_environment = new_env(0, NULL);

	Ast var;
	var = new_ast(0, NULL, AST_Integer, (uint64_t)VERSION);
	addenv_named(system_environment, 0, "VERSION", var);
	var = new_ast(0, NULL, AST_Integer, (uint64_t)RAND_MAX);
	addenv_named(system_environment, 0, "RAND_MAX", var);

	initialise_datatypes();
	initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	initialise_builtinop(operators         , builtinop, n_builtinop);
	initialise_errors();
	initialize_rand();

	return EXIT_SUCCESS;

	(void)builtin_is_Identifier__hidden;
}

