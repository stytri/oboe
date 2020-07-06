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

static inline int
is_AlphaNumeric(
	char32_t c
) {
	return is_UCS(UCS_Letter | UCS_Number, c);
}

static inline int
is_Alphabetic(
	char32_t c
) {
	return is_UCS(UCS_Letter, c);
}

static inline int
is_Numeric(
	char32_t c
) {
	return is_UCS(UCS_Number, c);
}

#define ENUM(Name,...)  \
static inline int \
is_##Name( \
	char32_t c \
) { \
	return is_UCS(UCS_##Name, c); \
}
ENUM(Letter)
ENUM(Mark)
ENUM(Number)
ENUM(Punctuation)
ENUM(Symbol)
ENUM(Separator)
ENUM(Other)
#undef ENUM

#define ENUM(Name,...)  \
static inline int \
is_Not##Name( \
	char32_t c \
) { \
	return !is_##Name(c); \
}
#include "system_ctype.enum"

//------------------------------------------------------------------------------

#define ENUM(Name,...)  static unsigned builtin_##Name##_enum = -1u;
	ENUM(to_Uppercase)
	ENUM(to_Lowercase)
#undef ENUM

#define ENUM(Name,...)  static unsigned builtin_span_##Name##_enum = -1u;
	ENUM(InSet)
#include "system_ctype.enum"

#define ENUM(Name,...)  static unsigned builtin_span_Rev##Name##_enum = -1u;
	ENUM(InSet)
#include "system_ctype.enum"

#define ENUM(Name,...)  static unsigned builtin_span_Not##Name##_enum = -1u;
	ENUM(InSet)
#include "system_ctype.enum"

#define ENUM(Name,...)  static unsigned builtin_span_RevNot##Name##_enum = -1u;
	ENUM(InSet)
#include "system_ctype.enum"

#define ENUM(Name,...)  static unsigned builtin_is_##Name##_enum = -1u;
	ENUM(CharInSet)
#include "system_ctype.enum"

#define ENUM(Name,...)  static unsigned builtin_is_Not##Name##_enum = -1u;
	ENUM(CharInSet)
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
	Ast range = ZEN;

	if(ast_isApplicate(arg) && ast_isRange(arg->m.rexpr)) {
		range = subeval(env, arg->m.rexpr);
		arg   = arg->m.lexpr;
	}
	arg = eval(env, arg);

	switch(arg->type) {
	case AST_Zen:
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Boolean, (uint64_t)is_ctype((char32_t)arg->m.ival));
	case AST_String: {
		int         res = 1;
		size_t      len;
		char const *cs  = StringToCharLiteral(arg->m.sval, &len);
		size_t      off = 0;
		size_t      end = 0;
		if(ast_isnotZen(range)) {
			off = ast_toInteger(range->m.lexpr);
			end = ast_toInteger(range->m.rexpr);
		}
		if(!end || end > len) {
			end = len - 1;
		}
		if((off < len) && (off <= end)) {
			if(off > 0) {
				end = utf8off(cs, NULL, end);
				off = utf8off(cs, &cs, off);
			}
			do {
				char32_t c = utf8chr(cs, &cs);
				res = is_ctype(c);
			} while(res && *cs && (off++ < end))
				;
			return new_ast(sloc, AST_Boolean, (uint64_t)res);
		}
		return oboerr(sloc, ERR_InvalidOperand);
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
builtin_is_##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_is_ctype(env, sloc, arg, is_##Name); \
}
#include "system_ctype.enum"

#define ENUM(Name,...) \
static Ast \
builtin_is_Not##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_is_ctype(env, sloc, arg, is_Not##Name); \
}
#include "system_ctype.enum"

//------------------------------------------------------------------------------

static Ast
builtin_is_CharInSet_delegate(
	Ast       env,
	sloc_t    sloc,
	Ast       args,
	bool      sense
) {
	if(ast_isSequence(args)) {
		Ast      ast = eval(env, args->m.lexpr);
		char32_t c   = (char32_t)ast_toInteger(ast);

		for(args = args->m.rexpr; ast_isSequence(args); args = args->m.rexpr) {

			ast = eval(env, args->m.lexpr);
			if(ast_isString(ast)) {
				char const *cs = StringToCharLiteral(ast->m.sval, NULL);
				if(utf8strchr(cs, c) != NULL) {
					return new_ast(sloc, AST_Boolean, (uint64_t)sense);
				}
			} else {
				if(c == ast_toInteger(ast)) {
					return new_ast(sloc, AST_Boolean, (uint64_t)sense);
				}
			}
		}

		ast = eval(env, args);
		if(ast_isString(ast)) {
			char const *cs = StringToCharLiteral(ast->m.sval, NULL);
			if(utf8strchr(cs, c) != NULL) {
				return new_ast(sloc, AST_Boolean, (uint64_t)sense);
			}
		} else {
			if(c == ast_toInteger(ast)) {
				return new_ast(sloc, AST_Boolean, (uint64_t)sense);
			}
		}

		return new_ast(sloc, AST_Boolean, (uint64_t)!sense);
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_is_CharInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_is_CharInSet_delegate(env, sloc, args, true);
}

static Ast
builtin_is_NotCharInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_is_CharInSet_delegate(env, sloc, args, false);
}

//------------------------------------------------------------------------------

static Ast
builtin_span_InSet_delegate(
	Ast       env,
	sloc_t    sloc,
	Ast       args,
	bool      sense
) {
	if(ast_isSequence(args)) {
		Ast range = ZEN;
		Ast lexpr = (ast_isApplicate(args->m.lexpr)
			&& ast_isRange(args->m.lexpr->m.rexpr)
		) ? (
			range = subeval(env, args->m.lexpr->m.rexpr),
			eval(env, args->m.lexpr->m.lexpr)
		):(
			eval(env, args->m.lexpr)
		);
		Ast qexpr = ZEN;
		Ast rexpr;

		if(ast_isSequence(args->m.rexpr)) {
			rexpr = eval(env, args->m.rexpr->m.lexpr);
			qexpr = eval(env, args->m.rexpr->m.rexpr);
		} else {
			rexpr = eval(env, args->m.rexpr);
		}

		char32_t q = '\0';

		if(ast_isnotZen(qexpr)) {
			if(ast_isString(qexpr)) {
				char const *cs = StringToCharLiteral(lexpr->m.sval, NULL);
				q = utf8chr(cs, &cs);
			} if(ast_isCharacter(qexpr)) {
				q = (char32_t)qexpr->m.ival;
			}
		}

		if(ast_isString(lexpr) && ast_isString(rexpr)) {
			uint64_t span = 0;

			size_t      len;
			char const *cs  = StringToCharLiteral(lexpr->m.sval, &len);
			char const *cz  = StringToCharLiteral(rexpr->m.sval, NULL);
			size_t      off = 0;
			size_t      end = 0;
			if(ast_isnotZen(range)) {
				off = ast_toInteger(range->m.lexpr);
				end = ast_toInteger(range->m.rexpr);
			}
			if(!end || end > len) {
				end = len - 1;
			}

			if((off < len) && (off <= end)) {
				if(off > 0) {
					end = utf8off(cs, NULL, end);
					off = utf8off(cs, &cs, off);
				}

				if(q != '\0') for(char32_t c; *cs && ~(c = utf8chr(cs, &cs)); ) {
					bool const quoted = (c == q);
					bool       inset = false;

					if(quoted) {
						if(!(*cs && ~(c = utf8chr(cs, &cs)))) {
							break;
						}

						span += quoted;
						span += quoted;
					} else {
						char const *ct = cz;
						for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
							if(c == t) {
								inset = true;
								span += !sense;
								break;
							}
						}

						if(inset == sense) {
							break;
						}

						span += sense;

						if(!(off++ < end)) {
							break;
						}
					}

				} else for(char32_t c; *cs && ~(c = utf8chr(cs, &cs)); ) {
					bool inset = false;

					char const *ct = cz;
					for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
						if(c == t) {
							inset = true;
							span += !sense;
							break;
						}
					}

					if(inset == sense) {
						break;
					}

					span += sense;

					if(!(off++ < end)) {
						break;
					}
				}
			}

			return new_ast(sloc, AST_Integer, span);
		}
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_span_InSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_span_InSet_delegate(env, sloc, args, false);
}

static Ast
builtin_span_NotInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_span_InSet_delegate(env, sloc, args, true);
}

//------------------------------------------------------------------------------

static Ast
builtin_span_RevInSet_delegate(
	Ast       env,
	sloc_t    sloc,
	Ast       args,
	bool      sense
) {
	if(ast_isSequence(args)) {
		Ast range = ZEN;
		Ast lexpr = (ast_isApplicate(args->m.lexpr)
			&& ast_isRange(args->m.lexpr->m.rexpr)
		) ? (
			range = subeval(env, args->m.lexpr->m.rexpr),
			eval(env, args->m.lexpr->m.lexpr)
		):(
			eval(env, args->m.lexpr)
		);
		Ast qexpr = ZEN;
		Ast rexpr;

		if(ast_isSequence(args->m.rexpr)) {
			rexpr = eval(env, args->m.rexpr->m.lexpr);
			qexpr = eval(env, args->m.rexpr->m.rexpr);
		} else {
			rexpr = eval(env, args->m.rexpr);
		}

		char32_t q = '\0';

		if(ast_isnotZen(qexpr)) {
			if(ast_isString(qexpr)) {
				char const *cs = StringToCharLiteral(lexpr->m.sval, NULL);
				q = utf8chr(cs, &cs);
			} if(ast_isCharacter(qexpr)) {
				q = (char32_t)qexpr->m.ival;
			}
		}

		if(ast_isString(lexpr) && ast_isString(rexpr)) {
			uint64_t span = 0;

			size_t      len;
			char const *cs = StringToCharLiteral(lexpr->m.sval, &len);
			char const *cz = StringToCharLiteral(rexpr->m.sval, NULL);
			size_t      off = 0;
			size_t      end = 0;
			if(ast_isnotZen(range)) {
				off = ast_toInteger(range->m.lexpr);
				end = ast_toInteger(range->m.rexpr);
			}
			if(!end || end > len) {
				end = len - 1;
			}

			if((off < len) && (off <= end)) {
				if(off > 0) {
					end = utf8off(cs, NULL, end);
					off = utf8off(cs, &cs, off);
				}

				char const *ce;
				bool        cont;

				if(q != '\0') do {
					bool inset;

					for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {
						bool const quoted = (c == q);

						inset = false;

						if(quoted) {
							if(!(*cs && ~(c = utf8chr(cs, &cs)))) {
								break;
							}
						} else {
							char const *ct = cz;
							for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
								if(c == t) {
									inset = true;
									break;
								}
							}
						}

						if(inset != sense) {
							cs = ce;
							break;
						}

						if(!(off++ < end)) {
							break;
						}
					}

					if(!(off < end)) {
						break;
					}

					cont = false;
					span = 0;

					for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {
						bool const quoted = (c == q);

						inset = false;

						if(quoted) {
							if(!(*cs && ~(c = utf8chr(cs, &cs)))) {
								break;
							}

							span += quoted;
							span += quoted;
						} else {
							char const *ct = cz;
							for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
								if(c == t) {
									inset = true;
									span += !sense;
									break;
								}
							}

							if(inset == sense) {
								cont = true;
								cs = ce;
								break;
							}

							span += sense;

							if(!(off++ < end)) {
								break;
							}
						}
					}

					if(!(off < end)) {
						break;
					}
				} while(cont)
					;
				else do {
					bool inset;

					for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {
						inset = false;

						char const *ct = cz;
						for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
							if(c == t) {
								inset = true;
								break;
							}
						}

						if(inset != sense) {
							cs = ce;
							break;
						}

						if(!(off++ < end)) {
							break;
						}
					}

					if(!(off < end)) {
						break;
					}

					cont = false;
					span = 0;

					for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {
						inset = false;

						char const *ct = cz;
						for(char32_t t; *ct && ~(t = utf8chr(ct, &ct)); ) {
							if(c == t) {
								inset = true;
								span += !sense;
								break;
							}
						}

						if(inset == sense) {
							cont = true;
							cs = ce;
							break;
						}

						span += sense;

						if(!(off++ < end)) {
							break;
						}
					}

					if(!(off < end)) {
						break;
					}
				} while(cont)
					;
			}

			return new_ast(sloc, AST_Integer, span);
		}
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_span_RevInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_span_RevInSet_delegate(env, sloc, args, false);
}

static Ast
builtin_span_RevNotInSet(
	Ast       env,
	sloc_t    sloc,
	Ast       args
) {
	return builtin_span_RevInSet_delegate(env, sloc, args, true);
}

//------------------------------------------------------------------------------

static Ast
builtin_span_ctype_delegate(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype,
	bool      sense
) {
	arg = eval(env, arg);
	if(ast_isString(arg)) {

		uint64_t span = 0;

		char const *cs = StringToCharLiteral(arg->m.sval, NULL);
		for(char32_t c; *cs && ~(c = utf8chr(cs, &cs)); ) {

			bool inset = is_ctype(c);
			if(inset == sense) {
				break;
			}

			span++;
		}

		return new_ast(sloc, AST_Integer, span);
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_span_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype
) {
	return builtin_span_ctype_delegate(env, sloc, arg, is_ctype, false);
}

#define ENUM(Name,...) \
static Ast \
builtin_span_##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_span_ctype(env, sloc, arg, is_##Name); \
}
#include "system_ctype.enum"

static Ast
builtin_span_not_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype
) {
	return builtin_span_ctype_delegate(env, sloc, arg, is_ctype, true);
}

#define ENUM(Name,...) \
static Ast \
builtin_span_Not##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_span_not_ctype(env, sloc, arg, is_##Name); \
}
#include "system_ctype.enum"

//------------------------------------------------------------------------------

static Ast
builtin_span_rev_ctype_delegate(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype,
	bool      sense
) {
	arg = eval(env, arg);
	if(ast_isString(arg)) {

		char const *cs = StringToCharLiteral(arg->m.sval, NULL);
		char const *ce;
		bool        inset;
		uint64_t    span;

		do {
			inset = sense;
			span  = 0;

			for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {

				inset = is_ctype(c);
				if(inset == sense) {
					cs = ce;
					break;
				}
			}

			for(char32_t c; *(ce = cs) && ~(c = utf8chr(cs, &cs)); ) {

				inset = is_ctype(c);
				if(inset != sense) {
					cs = ce;
					break;
				}

				span++;
			}

		} while(inset != sense)
			;

		return new_ast(sloc, AST_Integer, span);
	}

	return oboerr(sloc, ERR_InvalidOperand);
}

static Ast
builtin_span_rev_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype
) {
	return builtin_span_rev_ctype_delegate(env, sloc, arg, is_ctype, true);
}

#define ENUM(Name,...) \
static Ast \
builtin_span_Rev##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_span_rev_ctype(env, sloc, arg, is_##Name); \
}
#include "system_ctype.enum"

static Ast
builtin_span_rev_not_ctype(
	Ast       env,
	sloc_t    sloc,
	Ast       arg,
	is_CType  is_ctype
) {
	return builtin_span_rev_ctype_delegate(env, sloc, arg, is_ctype, false);
}

#define ENUM(Name,...) \
static Ast \
builtin_span_RevNot##Name( \
	Ast       env, \
	sloc_t    sloc, \
	Ast       arg \
) { \
	return builtin_span_rev_not_ctype(env, sloc, arg, is_##Name); \
}
#include "system_ctype.enum"

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
	case AST_Boolean:
	case AST_Integer:
	case AST_Character:
		return new_ast(sloc, AST_Character, to_ctype((char32_t)arg->m.ival));
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
	bool no_alias
) {
	static struct builtinfn const builtinfn[] = {
#	define STR(X)         #X

#	define ENUM(Name,...) BUILTIN(STR(Name), Name)
		ENUM(to_Uppercase)
		ENUM(to_Lowercase)
#	undef ENUM

#	define ENUM(Name,...) BUILTIN(STR(span_##Name), span_##Name)
		ENUM(InSet)
#	include "system_ctype.enum"

#	define ENUM(Name,...) BUILTIN(STR(span_Rev##Name), span_Rev##Name)
		ENUM(InSet)
#	include "system_ctype.enum"

#	define ENUM(Name,...) BUILTIN(STR(span_Not##Name), span_Not##Name)
		ENUM(InSet)
#	include "system_ctype.enum"

#	define ENUM(Name,...) BUILTIN(STR(span_RevNot##Name), span_RevNot##Name)
		ENUM(InSet)
#	include "system_ctype.enum"

#	define ENUM(Name,...) BUILTIN(STR(is_##Name), is_##Name)
		ENUM(CharInSet)
#	include "system_ctype.enum"

#	define ENUM(Name,...) BUILTIN(STR(is_Not##Name), is_Not##Name)
		ENUM(CharInSet)
#	include "system_ctype.enum"

#	undef STR
	};
	static size_t const n_builtinfn = sizeof(builtinfn) / sizeof(builtinfn[0]);

	static bool initialise = true;

	if(initialise) {
		initialise = false;

		initialise_builtinfn(system_environment, builtinfn, n_builtinfn);
	}

	return EXIT_SUCCESS;
	(void)no_alias;
}

