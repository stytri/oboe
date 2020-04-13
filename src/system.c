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
#include "odt.h"
#include "gc.h"
#include <stdlib.h>
#include <stdarg.h>
#include <locale.h>
#include <stdio.h>
#include <time.h>

//------------------------------------------------------------------------------

Ast system_environment = NULL;

//------------------------------------------------------------------------------

static unsigned builtin_file_type          = -1;
static unsigned builtin_fpos_type          = -1;

static unsigned builtin_sigil_enum         = -1;
static unsigned builtin_system_enum        = -1;
#define ENUM(Name,...) \
static unsigned builtin_is_##Name##_enum   = -1;
ENUM(Tag)
ENUM(Applicate)
ENUM(Array)
#include "oboe.enum"
static unsigned builtin_type_enum          = -1;
static unsigned builtin_type_name_enum     = -1;
static unsigned builtin_length_enum        = -1;
static unsigned builtin_to_String_enum     = -1;
static unsigned builtin_to_Literal_enum    = -1;
static unsigned builtin_to_Integer_enum    = -1;
static unsigned builtin_to_Float_enum      = -1;
static unsigned builtin_assert_enum        = -1;
static unsigned builtin_get_env_enum       = -1;
static unsigned builtin_set_locale_enum    = -1;
static unsigned builtin_clock_enum         = -1;
static unsigned builtin_time_enum          = -1;
static unsigned builtin_time_diff_enum     = -1;
static unsigned builtin_local_time_enum    = -1;
static unsigned builtin_utc_time_enum      = -1;
static unsigned builtin_eval_enum          = -1;
static unsigned builtin_parse_enum         = -1;
static unsigned builtin_load_enum          = -1;
static unsigned builtin_import_enum        = -1;
static unsigned builtin_temp_name_enum     = -1;
static unsigned builtin_rename_enum        = -1;
static unsigned builtin_remove_enum        = -1;
static unsigned builtin_is_file_enum       = -1;
static unsigned builtin_is_fpos_enum       = -1;
static unsigned builtin_open_enum          = -1;
static unsigned builtin_close_enum         = -1;
static unsigned builtin_flush_enum         = -1;
static unsigned builtin_rewind_enum        = -1;
static unsigned builtin_get_fpos_enum      = -1;
static unsigned builtin_set_fpos_enum      = -1;
static unsigned builtin_ferror_enum        = -1;
static unsigned builtin_fclear_enum        = -1;
static unsigned builtin_eof_enum           = -1;
static unsigned builtin_write_enum         = -1;
static unsigned builtin_write_line_enum    = -1;
static unsigned builtin_read_enum          = -1;
static unsigned builtin_read_line_enum     = -1;
static unsigned builtin_print_to_enum      = -1;
static unsigned builtin_print_line_to_enum = -1;
static unsigned builtin_print_enum         = -1;
static unsigned builtin_print_line_enum    = -1;
static unsigned builtin_print_error_enum   = -1;
static unsigned builtin_get_line_enum      = -1;
static unsigned builtin_exit_enum          = -1;

//------------------------------------------------------------------------------

static Ast
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

//------------------------------------------------------------------------------

static int
initialise_errors(
	void
) {
	static struct {
		char const *leme;
		Error       err;
	} builtinerr[] = {
#	define ENUM(Name,...)  { "ERROR "#Name, ERR_##Name },
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

static FILE *
builtin_file_type__open(
	char const *name,
	char const *mode
) {
	if(name && mode) {
		if(streq(name, "<")) {
			if(streq(mode, "r")) {
				return stdin;
			}
		} else if(streq(name, ">")) {
			if(streq(mode, "w")) {
				return stdout;
			}
		} else if(streq(name, ">>")) {
			if(streq(mode, "w")) {
				return stderr;
			}
		} else {
			return fopen(name, mode);
		}
	}

	return NULL;
}

static FILE *
builtin_file_type__close(
	FILE *file
) {
	if(file
		&& (file != stdin)
		&& (file != stdout)
		&& (file != stderr)
	) {
		fclose(file);
	}

	return NULL;
}

static Ast
builtin_file_type_new(
	Ast     ast,
	va_list va
) {
	String s;
	s                = va_arg(va, String);
	char const *name = StringToCharLiteral(s, NULL);
	s                = va_arg(va, String);
	char const *mode = StringToCharLiteral(s, NULL);
	FILE       *fp   = builtin_file_type__open(name, mode);
	if(fp) {
		ast->m.lptr = fp;
		return ast;
	}

	return oboerr(ast->sloc, ERR_InvalidOperand);
}

static Ast
builtin_file_type_eval(
	Ast ast
) {
	return ast;
}

static void
builtin_file_type_mark(
	Ast ast
) {
	(void)ast;
}

static void
builtin_file_type_sweep(
	Ast ast
) {
	if(ast->m.lptr) {
		ast->m.lptr = builtin_file_type__close(ast->m.lptr);
	}
}

static inline bool
ast_isFileReferenceType(
	Ast ast
) {
	return ast_isReferenceType(ast, builtin_file_type);
}

static inline bool
ast_isFileType(
	Ast ast
) {
	return ast_isType(ast, builtin_file_type);
}

static inline bool
ast_isFilePositionType(
	Ast ast
) {
	return ast_isReferenceType(ast, builtin_fpos_type);
}

//------------------------------------------------------------------------------

static Ast
builtin_fpos_type_new(
	Ast     ast,
	va_list va
) {
	static_assert(sizeof(fpos_t) <= sizeof(ast->m),
		"sizeof(fpos_t) <= sizeof(ast->m)"
	);

	fpos_t fp = va_arg(va, fpos_t);
	memcpy(&ast->m, &fp, sizeof(fp));
	return ast;
}

static Ast
builtin_fpos_type_eval(
	Ast ast
) {
	return ast;
}

static void
builtin_fpos_type_mark(
	Ast ast
) {
	(void)ast;
}

static void
builtin_fpos_type_sweep(
	Ast ast
) {
	(void)ast;
}

static inline bool
ast_isFilePosType(
	Ast ast
) {
	return ast_isType(ast, builtin_fpos_type);
}

//------------------------------------------------------------------------------

static int
initialise_datatypes(
	void
) {
	builtin_file_type = add_odt("file",
		builtin_file_type_new,
		builtin_file_type_eval,
		builtin_file_type_mark,
		builtin_file_type_sweep
	);

	builtin_fpos_type = add_odt("fpos",
		builtin_fpos_type_new,
		builtin_fpos_type_eval,
		builtin_fpos_type_mark,
		builtin_fpos_type_sweep
	);

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static Ast
dereference(
	Ast env,
	Ast arg
) {
	if(ast_isIdentifier(arg)) {
		arg = subeval(env, arg);
		if(ast_isReference(arg)) {
			arg = arg->m.rexpr;
		}
	}
	return arg;
}

static Ast
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
builtin_type_name(
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
builtin_get_env(
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
builtin_set_locale(
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
builtin_time_diff(
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
builtin_local_time(
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
builtin_utc_time(
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

		String s = mapoboefile(arg->m.sval);
		if(s) {
			arg = ZEN;

			char const *cs     = StringToCharLiteral(s, NULL);
			unsigned    source = sloc_source(sloc);

			for(size_t ts = gc_topof_stack();
				*cs;
			) {
				unsigned line = sloc_line(sloc);

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
builtin_temp_name(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isZen(arg)) {
		char const *cs = tmpnam(NULL);
		assert(cs != NULL);
		String      s  = CharLiteralToString(cs, strlen(cs));
		assert(s != NULL);

		return new_ast(sloc, NULL, AST_String, s);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
}

static Ast
builtin_rename_1(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isTag(arg)) {
		Ast from = eval(env, arg->m.lexpr);
		Ast to   = eval(env, arg->m.rexpr);
		if(ast_isString(from) && ast_isString(to)) {

			char const *from_file = StringToCharLiteral(from->m.sval, NULL);
			char const *to_file   = StringToCharLiteral(to->m.sval  , NULL);
			if(rename(from_file, to_file) == 0) {
				return ZEN;
			}

			return oboerr(sloc, ERR_FailedOperation);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_rename(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return sequential_evaluation(builtin_rename_1, env, sloc, arg);
}

//------------------------------------------------------------------------------

static Ast
builtin_remove_1(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval(env, arg);
	if(ast_isString(arg)) {

		char const *file = StringToCharLiteral(arg->m.sval, NULL);
		if(remove(file) == 0) {
			return ZEN;
		}

		return oboerr(sloc, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_remove(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return sequential_evaluation(builtin_remove_1, env, sloc, arg);
}

//------------------------------------------------------------------------------

static Ast
eval_file(
	Ast env,
	Ast arg
) {
	arg = eval(env, arg);
	if(ast_isFileReferenceType(arg)) {
		arg = arg->m.lexpr;
	}
	return arg;
}

static Ast
builtin_is_file(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = dereference(env, arg);
	uint64_t is = ast_isFileReferenceType(arg) || ast_isFileType(arg);
	return new_ast(sloc, NULL, AST_Integer, is);
	(void)env;
}

static Ast
builtin_is_fpos(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = dereference(env, arg);
	uint64_t is = ast_isFilePositionType(arg);
	return new_ast(sloc, NULL, AST_Integer, is);
	(void)env;
}

static Ast
builtin_open(
	Ast     env,
	sloc_t  sloc,
	Ast     arg
) {
	Ast    ast;
	String name;
	String mode;

	if(ast_isSequence(arg)) {
		ast = eval(env, arg->m.lexpr);
		if(!ast_isString(ast)) {
			return error_or(sloc, ast, ERR_InvalidOperand);
		}
		name = ast->m.sval;

		ast = eval(env, arg->m.rexpr);
		if(!ast_isString(ast)) {
			return error_or(sloc, ast, ERR_InvalidOperand);
		}
		mode = ast->m.sval;

	} else {
		ast = eval(env, arg);
		if(!ast_isString(ast)) {
			return error_or(sloc, ast, ERR_InvalidOperand);
		}
		name = ast->m.sval;
		mode = CharLiteralToString("r", 1);
	}

	ast  = new_ast(sloc, NULL, AST_OpaqueDataType, builtin_file_type, name, mode);
	return new_ast(sloc, NULL, AST_OpaqueDataReference, ast);
}

static Ast
builtin_close(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		arg->m.lptr = builtin_file_type__close(arg->m.lptr);
		return ZEN;
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_flush(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		fflush(arg->m.lptr);
		return ZEN;
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_rewind(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		rewind(arg->m.lptr);
		return ZEN;
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_get_fpos(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		fpos_t fp;
		if(fgetpos(arg->m.lptr, &fp) == 0)  {
			return new_ast(sloc, NULL, AST_OpaqueDataType, builtin_fpos_type, fp);
		}

		return error_or(sloc, arg, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_set_fpos(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isSequence(arg)) {
		Ast pos = eval(env, arg->m.rexpr);
		arg     = eval_file(env, arg->m.lexpr);

		if(ast_isFileType(arg) && (arg->m.lptr != NULL)
			&& ast_isFilePosType(pos)
		) {
			fpos_t *fpp = (fpos_t *)&pos->m;
			if(fsetpos(arg->m.lptr, fpp) == 0)  {
				return ZEN;
			}

			return error_or(sloc, arg, ERR_FailedOperation);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_ferror(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		uint64_t is_error = ferror(arg->m.lptr);
		return new_ast(sloc, NULL, AST_Integer, is_error);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_fclear(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		clearerr(arg->m.lptr);
		return ZEN;
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_eof(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		uint64_t is_eof = feof(arg->m.lptr);
		return new_ast(sloc, NULL, AST_Integer, is_eof);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static void
builtin_write_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	bool   archival
) {
	Ast    a = eval(env, arg);
	String s = tostr(a, archival);
	fputs(StringToCharLiteral(s, NULL), file->m.lptr);
	StringDelete(s);
	(void)sloc;
}

static Ast
builtin_write_all(
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	bool   archival,
	bool   line_per
) {
	if(ast_isSequence(arg)) {
		Ast file = eval_file(env, arg->m.lexpr);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {

			for(arg = arg->m.rexpr;
				ast_isSequence(arg);
				arg = arg->m.rexpr
			) {
				builtin_write_1(file, env, sloc, arg->m.lexpr, archival);
				if(line_per) {
					fputc('\n', file->m.lptr);
				}
			}

			builtin_write_1(file, env, sloc, arg, archival);
			fputc('\n', file->m.lptr);

			return ZEN;
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_write(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, true, true);
}

static Ast
builtin_write_line(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, true, false);
}

static int
builtin_read_1_char(
	void *f
) {
	int c = fgetc(f);
	return ((c == EOF) || (c == '\n')) ? 0 : c;
}

static Ast
builtin_read_1_parse(
	String s
) {
	Ast ast = ZEN;

	char const *args = StringToCharLiteral(s, NULL);
	unsigned    line = 1;

	size_t ts = gc_topof_stack();

	for(char const *cs = args; *cs; ) {

		ast = parse(cs, &cs, 0, &line, new_ast, false);
		if(ast_isnotZen(ast)) {
			ast = eval(globals, ast);
		}

		gc_return(ts, ast);
		run_gc();
	}

	return ast;
}

static Ast
builtin_read_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	Ast    ast = refeval(env, arg);
	String s   = StringBuild(builtin_read_1_char, file->m.lptr, 0);
	arg        = s ? (
		builtin_read_1_parse(s)
	) : (
		oboerr(sloc, ERR_FailedOperation)
	);

	if(ast_isnotZen(ast)) {
		return assign(sloc, ast, arg);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_read(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isSequence(arg)) {
		Ast file = eval_file(env, arg->m.lexpr);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {

			for(arg = arg->m.rexpr;
				ast_isSequence(arg);
				arg = arg->m.rexpr
			) {
				Ast ast = builtin_read_1(file, env, sloc, arg->m.lexpr);
				if(ast_isError(ast)) {
					return ast;
				}
			}

			return builtin_read_1(file, env, sloc, arg);
		}

		return error_or(sloc, file, ERR_InvalidOperand);

	} else {
		Ast file = eval_file(env, arg);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {
			arg = new_ast(sloc, NULL, AST_Void);

			return builtin_read_1(file, env, sloc, arg);
		}

		return error_or(sloc, file, ERR_InvalidOperand);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_read_line(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	Ast file = eval_file(env, arg);
	if(ast_isFileType(file) && (file->m.lptr != NULL)) {

		String s = StringBuild(builtin_read_1_char, file->m.lptr, 0);
		if(s) {
			return new_ast(sloc, NULL, AST_String, s);
		}

		return oboerr(sloc, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_print_to(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, false, true);
}

static Ast
builtin_print_line_to(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, false, false);
}

//------------------------------------------------------------------------------

static void
builtin_print_1(
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	FILE  *out
) {
	Ast    a = eval(env, arg);
	String s = tostr(a, false);
	fputs(StringToCharLiteral(s, NULL), out);
	StringDelete(s);
	(void)sloc;
}

static Ast
builtin_print_all(
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	FILE  *out
) {
	for(;
		ast_isSequence(arg);
		arg = arg->m.rexpr
	) {
		builtin_print_1(env, sloc, arg->m.lexpr, out);
	}
	if(ast_isnotZen(arg)) {
		builtin_print_1(env, sloc, arg, out);
	}

	return ZEN;
	(void)sloc;
}

static Ast
builtin_print(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_print_all(env, sloc, arg, stdout);
}

static Ast
builtin_print_line(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = builtin_print_all(env, sloc, arg, stdout);
	fputc('\n', stdout);
	return arg;
}

static Ast
builtin_print_error(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	fflush(stdout);
	arg = builtin_print_all(env, sloc, arg, stderr);
	fputc('\n', stderr);
	fflush(stderr);
	return arg;
}

static Ast
builtin_get_line(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isZen(arg)) {

		String s = StringBuild(builtin_read_1_char, stdin, 0);
		if(s) {
			return new_ast(sloc, NULL, AST_String, s);
		}

		return oboerr(sloc, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
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
			arg = builtin_print_line(env, sloc, arg);
			exit(EXIT_FAILURE);
		}
	}

	exit(EXIT_SUCCESS);
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
		BUILTIN("system"       , system)
#		define ENUM(Name,...)  \
		BUILTIN("is_"#Name     , is_##Name)
		ENUM(Tag)
		ENUM(Applicate)
		ENUM(Array)
#		include "oboe.enum"
		BUILTIN("type"         , type)
		BUILTIN("type_name"    , type_name)
		BUILTIN("length"       , length)
		BUILTIN("to_String"    , to_String)
		BUILTIN("to_Literal"   , to_Literal)
		BUILTIN("to_Integer"   , to_Integer)
		BUILTIN("to_Float"     , to_Float)
		BUILTIN("assert"       , assert)
		BUILTIN("get_env"      , get_env)
		BUILTIN("set_locale"   , set_locale)
		BUILTIN("clock"        , clock)
		BUILTIN("time"         , time)
		BUILTIN("time_diff"    , time_diff)
		BUILTIN("local_time"   , local_time)
		BUILTIN("utc_time"     , utc_time)
		BUILTIN("eval"         , eval)
		BUILTIN("parse"        , parse)
		BUILTIN("load"         , load)
		BUILTIN("import"       , import)
		BUILTIN("temp_name"    , temp_name)
		BUILTIN("rename"       , rename)
		BUILTIN("remove"       , remove)
		BUILTIN("is_file"      , is_file)
		BUILTIN("is_fpos"      , is_fpos)
		BUILTIN("open"         , open)
		BUILTIN("close"        , close)
		BUILTIN("flush"        , flush)
		BUILTIN("rewind"       , rewind)
		BUILTIN("get_fpos"     , get_fpos)
		BUILTIN("set_fpos"     , set_fpos)
		BUILTIN("ferror"       , ferror)
		BUILTIN("fclear"       , fclear)
		BUILTIN("eof"          , eof)
		BUILTIN("write"        , write)
		BUILTIN("write_line"   , write_line)
		BUILTIN("read"         , read)
		BUILTIN("read_line"    , read_line)
		BUILTIN("print_to"     , print_to)
		BUILTIN("print_line_to", print_line_to)
		BUILTIN("print"        , print)
		BUILTIN("print_line"   , print_line)
		BUILTIN("print_error"  , print_error)
		BUILTIN("get_line"     , get_line)
		BUILTIN("exit"         , exit)
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	system_environment = new_env(0, NULL);

	Ast version = new_ast(0, NULL, AST_Integer, (uint64_t)VERSION);
	addenv_named(system_environment, 0, "VERSION", version);

	initialise_datatypes();
	initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	initialise_builtinop(operators         , builtinop, n_builtinop);
	initialise_errors();

	return EXIT_SUCCESS;

	(void)builtin_is_Identifier__hidden;
}

