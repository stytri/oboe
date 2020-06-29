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
#include "nobreak.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

//------------------------------------------------------------------------------

static unsigned builtin_file_type      = -1u;
static unsigned builtin_fpos_type      = -1u;

static unsigned builtin_tmpname_enum   = -1u;
static unsigned builtin_rename_enum    = -1u;
static unsigned builtin_remove_enum    = -1u;
static unsigned builtin_is_File_enum   = -1u;
static unsigned builtin_is_Fpos_enum   = -1u;
static unsigned builtin_open_enum      = -1u;
static unsigned builtin_close_enum     = -1u;
static unsigned builtin_flush_enum     = -1u;
static unsigned builtin_rewind_enum    = -1u;
static unsigned builtin_getpos_enum    = -1u;
static unsigned builtin_setpos_enum    = -1u;
static unsigned builtin_ferror_enum    = -1u;
static unsigned builtin_fclear_enum    = -1u;
static unsigned builtin_eof_enum       = -1u;
static unsigned builtin_wr_u8_enum     = -1u;
static unsigned builtin_wr_u16_enum    = -1u;
static unsigned builtin_wr_u32_enum    = -1u;
static unsigned builtin_wr_u64_enum    = -1u;
static unsigned builtin_wr_f32_enum    = -1u;
static unsigned builtin_wr_f64_enum    = -1u;
static unsigned builtin_wr_str_enum    = -1u;
static unsigned builtin_rd_u8_enum     = -1u;
static unsigned builtin_rd_u16_enum    = -1u;
static unsigned builtin_rd_u32_enum    = -1u;
static unsigned builtin_rd_u64_enum    = -1u;
static unsigned builtin_rd_f32_enum    = -1u;
static unsigned builtin_rd_f64_enum    = -1u;
static unsigned builtin_rd_str_enum    = -1u;
static unsigned builtin_write_enum     = -1u;
static unsigned builtin_read_enum      = -1u;
static unsigned builtin_fprint_enum    = -1u;
static unsigned builtin_fprintln_enum  = -1u;
static unsigned builtin_fgetln_enum    = -1u;
static unsigned builtin_print_enum     = -1u;
static unsigned builtin_println_enum   = -1u;
static unsigned builtin_printerr_enum  = -1u;
static unsigned builtin_getln_enum     = -1u;

//------------------------------------------------------------------------------

typedef float  float32_t;
typedef double float64_t;

union btypes {
	uint8_t  uint8;
	uint16_t uint16;
	uint32_t uint32;
	uint64_t uint64;
	float    float32;
	double   float64;
};

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
	ast->m.lptr      = builtin_file_type__open(name, mode);
	if(ast->m.lptr) {
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
	Ast    ast,
	void (*gc_mark)(void const *)
) {
	(void)ast;
	(void)gc_mark;
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
	Ast    ast,
	void (*gc_mark)(void const *)
) {
	(void)ast;
	(void)gc_mark;
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
	static bool initialise = true;

	if(initialise) {
		initialise = false;

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
	}

	return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------

static Ast
builtin_tmpname(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isZen(arg)) {
		char const *cs = tmpnam(NULL);
		assert(cs != NULL);
		String      s  = CharLiteralToString(cs, strlen(cs));
		assert(s != NULL);

		return new_ast(sloc, AST_String, s);
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
builtin_is_File(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = dereference(env, arg);
	uint64_t is = ast_isFileReferenceType(arg) || ast_isFileType(arg);
	return new_ast(sloc, AST_Boolean, is);
	(void)env;
}

static Ast
builtin_is_Fpos(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = dereference(env, arg);
	uint64_t is = ast_isFilePositionType(arg);
	return new_ast(sloc, AST_Boolean, is);
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

	ast  = new_ast(sloc, AST_OpaqueDataType, builtin_file_type, name, mode);
	return new_ast(sloc, AST_OpaqueDataReference, ast);
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
builtin_getpos(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = eval_file(env, arg);
	if(ast_isFileType(arg) && (arg->m.lptr != NULL)) {
		fpos_t fp;
		if(fgetpos(arg->m.lptr, &fp) == 0)  {
			return new_ast(sloc, AST_OpaqueDataType, builtin_fpos_type, fp);
		}

		return error_or(sloc, arg, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_setpos(
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
		uint64_t is_error = (uint64_t)ferror(arg->m.lptr);
		return new_ast(sloc, AST_Boolean, is_error);
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
		uint64_t is_eof = (uint64_t)feof(arg->m.lptr);
		return new_ast(sloc, AST_Boolean, is_eof);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

#define WR1(Type,Value) {\
	bval.Type = (Type##_t)Value; \
	res = fwrite(&bval.Type, sizeof(bval.Type), 1, file->m.lptr); \
}

static Ast
builtin_wr_u_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t res = 0;

	arg = eval(env, arg);
	switch(arg->type) {
		union btypes bval;
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		switch(bits) {
		case 8 : WR1(uint8 , arg->m.ival); break;
		case 16: WR1(uint16, arg->m.ival); break;
		case 32: WR1(uint32, arg->m.ival); break;
		default:
		case 64: WR1(uint64, arg->m.ival); break;
		}
		break;
	case AST_Float:
		switch(bits) {
		case 8 : WR1(uint8 , arg->m.fval); break;
		case 16: WR1(uint16, arg->m.fval); break;
		case 32: WR1(uint32, arg->m.fval); break;
		default:
		case 64: WR1(uint64, arg->m.fval); break;
		}
		break;
	default:
		return error_or(sloc, arg, ERR_InvalidOperand);
	}

	if(res > 0) {
		return ZEN;
	}

	return oboerr(sloc, ERR_FailedOperation);
}

static Ast
builtin_wr_f_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t res = 0;

	arg = eval(env, arg);
	switch(arg->type) {
		union btypes bval;
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		switch(bits) {
		case 32: WR1(float32, arg->m.ival); break;
		default:
		case 64: WR1(float64, arg->m.ival); break;
		}
		break;
	case AST_Float:
		switch(bits) {
		case 32: WR1(float32, arg->m.fval); break;
		default:
		case 64: WR1(float64, arg->m.fval); break;
		}
		break;
	default:
		return error_or(sloc, arg, ERR_InvalidOperand);
	}

	if(res > 0) {
		return ZEN;
	}

	return oboerr(sloc, ERR_FailedOperation);
}

static Ast
builtin_wr_s_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t res = 0;
	bool   local_string = false;

	arg = eval(env, arg);
	switch(arg->type) {
		union btypes bval;
		StringConst s;
		size_t      len;
		char const *cs;
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
	case AST_Float:
		s = tostr(arg, false);
		assert(s != NULL);
		local_string = true;
		nobreak;
	case AST_String:
		if(!local_string) {
			s = arg->m.sval;
		}
		cs = StringToCharLiteral(s, &len);
		WR1(uint32, len);
		if(res > 0 && len > 0) {
			size_t res1 = res;
			res = fwrite(cs, sizeof(*cs), len, file->m.lptr);
			if(res > 0) {
				res += res1;
			}
		}
		if(local_string) {
			StringDelete(s);
		}
		break;
	default:
		return error_or(sloc, arg, ERR_InvalidOperand);
	}

	if(res > 0) {
		return ZEN;
	}

	return oboerr(sloc, ERR_FailedOperation);
	(void)bits;
}

static Ast
builtin_wr_all(
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits,
	Ast  (*wr1)(
		Ast    file,
		Ast    env,
		sloc_t sloc,
		Ast    arg,
		int    bits
	)
) {
	if(ast_isSequence(arg)) {
		Ast file = eval_file(env, arg->m.lexpr);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {

			for(arg = arg->m.rexpr;
				ast_isSequence(arg);
				arg = arg->m.rexpr
			) {
				Ast res = wr1(file, env, sloc, arg->m.lexpr, bits);
				if(ast_isnotZen(res)) {
					return res;
				}
			}

			return wr1(file, env, sloc, arg, bits);
		}
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_wr_u8(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 8, builtin_wr_u_1);
}
static Ast
builtin_wr_u16(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 16, builtin_wr_u_1);
}
static Ast
builtin_wr_u32(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 32, builtin_wr_u_1);
}
static Ast
builtin_wr_u64(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 64, builtin_wr_u_1);
}

static Ast
builtin_wr_f32(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 32, builtin_wr_f_1);
}
static Ast
builtin_wr_f64(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 64, builtin_wr_f_1);
}

static Ast
builtin_wr_str(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_wr_all(env, sloc, arg, 64, builtin_wr_s_1);
}

#undef WR1

//------------------------------------------------------------------------------

#define RD1(Type,Value) {\
	res = fread(&bval.Type, sizeof(bval.Type), 1, file->m.lptr); \
	Value = bval.Type; \
}

static Ast
builtin_rd_u_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t       res = 0;
	union btypes bval;

	switch(bits) {
	case 8 : RD1(uint8 , bval.uint64); break;
	case 16: RD1(uint16, bval.uint64); break;
	case 32: RD1(uint32, bval.uint64); break;
	default:
	case 64: RD1(uint64, bval.uint64); break;
	}

	if(res > 0) {
		if(ast_isZen(arg)) {
			return new_ast(sloc, AST_Integer, bval.uint64);
		}

		Ast ast = subeval(env, arg);
		if(ast_isReference(ast)) {
			arg = new_ast(sloc, AST_Integer, bval.uint64);
			ast = unwrapref(ast);

			return assign(sloc, &ast->m.rexpr, arg);
		}

		return error_or(sloc, arg, ERR_InvalidReferent);
	}

	return oboerr(sloc, ERR_FailedOperation);
}

static Ast
builtin_rd_f_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t       res = 0;
	union btypes bval;

	switch(bits) {
	case 32: RD1(float32, bval.float64); break;
	default:
	case 64: RD1(float64, bval.float64); break;
	}

	if(res > 0) {
		if(ast_isZen(arg)) {
			return new_ast(sloc, AST_Float, bval.float64);
		}

		Ast ast = subeval(env, arg);
		if(ast_isReference(ast)) {
			arg = new_ast(sloc, AST_Float, bval.float64);
			ast = unwrapref(ast);

			return assign(sloc, &ast->m.rexpr, arg);
		}

		return error_or(sloc, arg, ERR_InvalidReferent);
	}

	return oboerr(sloc, ERR_FailedOperation);
}

static Ast
builtin_rd_s_1(
	Ast    file,
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits
) {
	size_t       res = 0;
	union btypes bval;
	size_t       len;
	String       s;

	RD1(uint32, len);
	if(res > 0) {
		s = StringReserve(len);
		assert(s != NULL);

		if(len > 0) {
			char  *cs   = (char *)StringToCharLiteral(s, &len);
			size_t res1 = res;
			res = fread(cs, sizeof(*cs), len, file->m.lptr);
			if(res > 0) {
				res += res1;
			}
		}
	}
	if(res > 0) {
		if(ast_isZen(arg)) {
			return new_ast(sloc, AST_String, s);
		}

		Ast ast = subeval(env, arg);
		if(ast_isReference(ast)) {
			arg = new_ast(sloc, AST_String, s);
			ast = unwrapref(ast);

			return assign(sloc, &ast->m.rexpr, arg);
		}

		return error_or(sloc, arg, ERR_InvalidReferent);
	}

	return oboerr(sloc, ERR_FailedOperation);
	(void)bits;
}

static Ast
builtin_rd_all(
	Ast    env,
	sloc_t sloc,
	Ast    arg,
	int    bits,
	Ast  (*rd1)(
		Ast    file,
		Ast    env,
		sloc_t sloc,
		Ast    arg,
		int    bits
	)
) {
	if(ast_isSequence(arg)) {
		Ast file = eval_file(env, arg->m.lexpr);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {

			for(arg = arg->m.rexpr;
				ast_isSequence(arg);
				arg = arg->m.rexpr
			) {
				Ast ast = rd1(file, env, sloc, arg->m.lexpr, bits);
				if(ast_isError(ast)) {
					return ast;
				}
			}

			return rd1(file, env, sloc, arg, bits);
		}

		return error_or(sloc, file, ERR_InvalidOperand);

	} else {
		Ast file = eval_file(env, arg);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {
			return rd1(file, env, sloc, ZEN, bits);
		}

		return error_or(sloc, file, ERR_InvalidOperand);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

static Ast
builtin_rd_u8(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 8, builtin_rd_u_1);
}
static Ast
builtin_rd_u16(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 16, builtin_rd_u_1);
}
static Ast
builtin_rd_u32(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 32, builtin_rd_u_1);
}
static Ast
builtin_rd_u64(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 64, builtin_rd_u_1);
}

static Ast
builtin_rd_f32(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 32, builtin_rd_f_1);
}
static Ast
builtin_rd_f64(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 64, builtin_rd_f_1);
}

static Ast
builtin_rd_str(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_rd_all(env, sloc, arg, 64, builtin_rd_s_1);
}

#undef RD1

//------------------------------------------------------------------------------

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
	bool   end_line
) {
	if(ast_isSequence(arg)) {
		Ast file = eval_file(env, arg->m.lexpr);
		if(ast_isFileType(file) && (file->m.lptr != NULL)) {

			for(arg = arg->m.rexpr;
				ast_isSequence(arg);
				arg = arg->m.rexpr
			) {
				builtin_write_1(file, env, sloc, arg->m.lexpr, archival);
				if(archival) {
					fputc('\n', file->m.lptr);
				}
			}

			builtin_write_1(file, env, sloc, arg, archival);
			if(archival || end_line) {
				fputc('\n', file->m.lptr);
			}

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

//------------------------------------------------------------------------------

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

	char const   *args = StringToCharLiteral(s, NULL);
	unsigned long line = 1;

	size_t ts = gc_topof_stack();

	for(char const *cs = args; *cs; ) {

		ast = parse(cs, &cs, 0, &line, new_ast_from_lexeme, false);
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
	Ast    ast = subeval(env, arg);
	String s   = StringBuild(builtin_read_1_char, file->m.lptr, 0);
	arg        = s ? (
		builtin_read_1_parse(s)
	) : (
		oboerr(sloc, ERR_FailedOperation)
	);

	if(ast_isReference(ast)) {
		ast = unwrapref(ast);

		return assign(sloc, &ast->m.rexpr, arg);
	}
	if(ast_isZen(ast)) {
		return arg;
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
			return builtin_read_1(file, env, sloc, ZEN);
		}

		return error_or(sloc, file, ERR_InvalidOperand);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
}

//------------------------------------------------------------------------------

static Ast
builtin_fprint(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, false, false);
}

static Ast
builtin_fprintln(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	return builtin_write_all(env, sloc, arg, false, true);
}

static Ast
builtin_fgetln(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	Ast file = eval_file(env, arg);
	if(ast_isFileType(file) && (file->m.lptr != NULL)) {

		String s = StringBuild(builtin_read_1_char, file->m.lptr, 0);
		if(s) {
			return new_ast(sloc, AST_String, s);
		}

		return oboerr(sloc, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
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
builtin_println(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	arg = builtin_print_all(env, sloc, arg, stdout);
	fputc('\n', stdout);
	return arg;
}

Ast
builtin_printerr(
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
builtin_getln(
	Ast    env,
	sloc_t sloc,
	Ast    arg
) {
	if(ast_isZen(arg)) {

		String s = StringBuild(builtin_read_1_char, stdin, 0);
		if(s) {
			return new_ast(sloc, AST_String, s);
		}

		return oboerr(sloc, ERR_FailedOperation);
	}

	return error_or(sloc, arg, ERR_InvalidOperand);
	(void)env;
}

//------------------------------------------------------------------------------

int
initialise_system_stdio(
	bool no_alias
) {
	static struct builtinfn const builtinfn[] = {
		BUILTIN("tmpname" , tmpname)
		BUILTIN("rename"  , rename)
		BUILTIN("remove"  , remove)
		BUILTIN("is_File" , is_File)
		BUILTIN("is_Fpos" , is_Fpos)
		BUILTIN("open"    , open)
		BUILTIN("close"   , close)
		BUILTIN("flush"   , flush)
		BUILTIN("rewind"  , rewind)
		BUILTIN("getpos"  , getpos)
		BUILTIN("setpos"  , setpos)
		BUILTIN("ferror"  , ferror)
		BUILTIN("fclear"  , fclear)
		BUILTIN("eof"     , eof)
		BUILTIN("wr_u8"   , wr_u8)
		BUILTIN("wr_u16"  , wr_u16)
		BUILTIN("wr_u32"  , wr_u32)
		BUILTIN("wr_u64"  , wr_u64)
		BUILTIN("wr_f32"  , wr_f32)
		BUILTIN("wr_f64"  , wr_f64)
		BUILTIN("wr_str"  , wr_str)
		BUILTIN("rd_u8"   , rd_u8)
		BUILTIN("rd_u16"  , rd_u16)
		BUILTIN("rd_u32"  , rd_u32)
		BUILTIN("rd_u64"  , rd_u64)
		BUILTIN("rd_f32"  , rd_f32)
		BUILTIN("rd_f64"  , rd_f64)
		BUILTIN("rd_str"  , rd_str)
		BUILTIN("write"   , write)
		BUILTIN("read"    , read)
		BUILTIN("fprint"  , fprint)
		BUILTIN("fprintln", fprintln)
		BUILTIN("fgetln"  , fgetln)
		BUILTIN("print"   , print)
		BUILTIN("println" , println)
		BUILTIN("printerr", printerr)
		BUILTIN("getln"   , getln)
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		initialise_datatypes();
		initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	}

	return EXIT_SUCCESS;
	(void)no_alias;
}

