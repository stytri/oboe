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
#include "tostr.h"
#include "strlib.h"
#include "sources.h"
#include "env.h"
#include "odt.h"
#include "builtins.h"
#include "utf8.h"
#include <stdio.h>

//------------------------------------------------------------------------------

static String
toString(
	String s,
	bool   archival,
	Ast    ast
);

static String
toString_integer(
	String      s,
	bool        archival,
	char const *prefix,
	uint64_t    value,
	char const *postfix
) {
	char        temp[(CHAR_BIT * sizeof(value)) + 1];
	char const *fmt  = archival ? "0x%16.16"PRIX64 : "%"PRIu64;
	int         n    = snprintf(temp, sizeof(temp)-1, fmt, value);

	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(n > 0) {
		s = StringAppendCharLiteral(s, temp, (size_t)n);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
	(void)archival;
}

static String
toString_float(
	String      s,
	bool        archival,
	char const *prefix,
	double      value,
	char const *postfix
) {
	char        temp[(CHAR_BIT * sizeof(value)) + 1];
	char const *fmt  = archival ? "%#.13a" : "%#g";
	int         n    = snprintf(temp, sizeof(temp)-1, fmt, value);

	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(n > 0) {
		s = StringAppendCharLiteral(s, temp, (size_t)n);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_character(
	String      s,
	bool        archival,
	char const *prefix,
	uint64_t    value,
	char const *postfix
) {
	char   temp[8] = { '\0' };
	size_t len     = utf8encode(temp, NULL, (char32_t)value);
	temp[len]      = '\0';

	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(archival) {
		s = StringAppendChar(s, '\'');
		s = EscapeString(s, temp, NULL, "'", '\0');
		s = StringAppendChar(s, '\'');
	} else {
		s = StringAppendCharLiteral(s, temp, len);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_string(
	String      s,
	bool        archival,
	char const *prefix,
	String      value,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(archival) {
		s = StringAppendChar(s, '"');
		s = EscapeString(s, StringToCharLiteral(value, NULL), NULL, "\"", '\0');
		s = StringAppendChar(s, '"');
	} else {
		s = StringAppend(s, value);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_identifier(
	String      s,
	bool        archival,
	char const *prefix,
	String      value,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(s) {
		s = StringAppend(s, value);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
	(void)archival;
}

static String
toString_lexeme(
	String      s,
	bool        archival,
	char const *prefix,
	char const *value,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	s = StringAppendCharLiteral(s, value, strlen(value));
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
	(void)archival;
}

static String
toString_expression(
	String      s,
	bool        archival,
	char const *prefix,
	Ast         value,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(ast_isnotZen(value)) {
		s = toString(s, archival, value);
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_operator(
	String      s,
	bool        archival,
	char const *prefix,
	Ast         lexpr,
	char const *value,
	Ast         rexpr,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(ast_isnotZen(lexpr)) {
		s = toString_expression(s, archival, "(", lexpr, ")");
	}
	if(value && *value) {
		s = StringAppendCharLiteral(s, value, strlen(value));
	}
	if(ast_isnotZen(rexpr)) {
		s = toString_expression(s, archival, "(", rexpr, ")");
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_sequence(
	String      s,
	bool        archival,
	char const *prefix,
	Ast         lexpr,
	char const *value,
	Ast         rexpr,
	char const *postfix
) {
	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(ast_isnotZen(lexpr)) {
		s = toString_expression(s, archival, "(", lexpr, ")");
	}
	s = StringAppendCharLiteral(s, value, strlen(value));
	if(ast_isnotZen(rexpr)) {
		s = toString_expression(s, archival, "(", rexpr, ")");
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_environment(
	String      s,
	bool        archival,
	char const *prefix,
	Ast         ast,
	char const *postfix
) {
	Array env = ast->m.env;

	s = StringAppendCharLiteral(s, prefix, strlen(prefix));
	if(env) {
		size_t const n = array_length(env);
		if(n > 0) {
			Ast value = array_at(env, Ast, 0);
			if(ast_isReference(value)) {
				s = toString_expression(s, archival, "('", value, "':");
				s = toString_expression(s, archival, "(" , value->m.rexpr, "))");
			} else {
				s = toString_expression(s, archival, "(", value, ")");
			}
			for(size_t i = 1; i < n; ++i) {
				value = array_at(env, Ast, i);
				if(ast_isReference(value)) {
					s = toString_expression(s, archival, ",('", value, "':");
					s = toString_expression(s, archival,  "(" , value->m.rexpr, "))");
				} else {
					s = toString_expression(s, archival, ",(", value, ")");
				}
			}
		}
	}
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
}

static String
toString_error(
	String      s,
	bool        archival,
	char const *prefix,
	Ast         err,
	char const *postfix
) {
	char const *errs   = oboerrs    (err->qual);
	unsigned    srcid  = sloc_source(err->sloc);
	String      srcs   = get_source (srcid);
	char        line  [(CHAR_BIT * sizeof(unsigned)) + 1];
	char        offset[(CHAR_BIT * sizeof(unsigned)) + 1];
	char        count [(CHAR_BIT * sizeof(unsigned)) + 1];

	char const *source = StringToCharLiteral(srcs, NULL);
	snprintf(line  , sizeof(line)  , ":%lu" , sloc_line  (err->sloc));
	snprintf(offset, sizeof(offset), ":%u"  , sloc_offset(err->sloc)+1u);
	snprintf(count , sizeof(count) , ":%u: ", sloc_count (err->sloc));

	s = StringAppendCharLiteral(s, prefix , strlen(prefix));
	s = StringAppendCharLiteral(s, source , strlen(source));
	s = StringAppendCharLiteral(s, line   , strlen(line));
	s = StringAppendCharLiteral(s, offset , strlen(offset));
	s = StringAppendCharLiteral(s, count  , strlen(count));
	s = StringAppendCharLiteral(s, errs   , strlen(errs));
	s = StringAppendCharLiteral(s, postfix, strlen(postfix));

	return s;
	(void)archival;
}

static String
toString(
	String s,
	bool   archival,
	Ast    ast
) {
	if(s) switch(ast_type(ast)) {
	default: {
#	define ENUM(Name) } break; case AST_##Name: {
#	define STR(...)  __VA_ARGS__;

#	define INTEGER(Pre,Val,Post)               s = toString_integer    (s, archival, (Pre), (Val), (Post))
#	define FLOAT(Pre,Val,Post)                 s = toString_float      (s, archival, (Pre), (Val), (Post))
#	define CHARACTER(Pre,Val,Post)             s = toString_character  (s, archival, (Pre), (Val), (Post))
#	define STRING(Pre,Val,Post)                s = toString_string     (s, archival, (Pre), (Val), (Post))
#	define IDENTIFIER(Pre,Val,Post)            s = toString_identifier (s, archival, (Pre), (Val), (Post))
#	define LEXEME(Pre,Val,Post)                s = toString_lexeme     (s, archival, (Pre), (Val), (Post))
#	define OPERATOR(Pre,Lexpr,Val,Rexpr,Post)  s = toString_operator   (s, archival, (Pre), (Lexpr), (Val), (Rexpr), (Post))
#	define SEQUENCE(Pre,Lexpr,Val,Rexpr,Post)  s = toString_sequence   (s, archival, (Pre), (Lexpr), (Val), (Rexpr), (Post))
#	define EXPRESSION(Pre,Val,Post)            s = toString_expression (s, archival, (Pre), (Val), (Post))
#	define ENVIRONMENT(Pre,Array,Post)         s = toString_environment(s, archival, (Pre), (Array), (Post))
#	define ERRORMESSAGE(Pre,Err,Post)          s = toString_error      (s, archival, (Pre), (Err), (Post))
#	define OPERATOROF(Qual)                    getops(Qual)

#	include "oboe.enum"

#	undef INTEGER
#	undef FLOAT
#	undef CHARACTER
#	undef STRING
#	undef IDENTIFIER
#	undef LEXEME
#	undef OPERATOR
#	undef SEQUENCE
#	undef EXPRESSION
#	undef ENVIRONMENT
#	undef ERRORMESSAGE
#	undef OPERATOROF
	}}

	return s;
}

//------------------------------------------------------------------------------

String
tostr(
	Ast  ast,
	bool archival
) {
	return toString(StringCreate(), archival, ast);
}

