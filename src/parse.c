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
#include "parse.h"
#include "lex.h"
#include "env.h"
#include "hash.h"
#include "utf8.h"
#include <stdbool.h>

//------------------------------------------------------------------------------

Precedence
precedence(
	char const *cs,
	size_t      n
) {
	uint64_t hash  = memhash(cs, n, 0);
	size_t   index = locate(operators, hash, cs, n);

	Ast opr = getopr(index);

	return (opr != ZEN) ? opr->qual : P_None;
}

//------------------------------------------------------------------------------
/*

primary:    '(' assemblage ')'
            '[' assemblage ']'
            '{' assemblage '}'
            Integer
            Float
            Character
            String
            Identifier

applicate:  primary  [ primary ]*
            Operator [ primary ]*

operation:  applicate [ Operator applicate ]*

sequence:   operation [ ',' operation ]*

assemblage: sequence  [ ';' sequence ]*

*/
//------------------------------------------------------------------------------

typedef struct parse_state *ParseState;
struct parse_state {
	char const  *leme;
	size_t       len;
	char const  *cs;
	unsigned     source;
	char const  *startofline;
	unsigned    *linop;
};

//------------------------------------------------------------------------------

static inline sloc_t
parse_sloc(
	ParseState ps
) {
	return make_sloc(ps->source, *ps->linop, ps->leme - ps->startofline, ps->len);
}

static char const *
parse_peek(
	ParseState ps
) {
	if(!ps->leme) {
		ps->leme = lex(ps->cs, &ps->cs, &ps->startofline, ps->linop);
		ps->len  = ps->cs - ps->leme;
	}

	return ps->leme;
}

static char const *
parse_get(
	ParseState ps,
	size_t    *lenp
) {
	char const *leme = parse_peek(ps);
	*lenp = ps->len;

	ps->leme = NULL;
	ps->len  = 0;

	return leme;
}

static inline char const *
parse_accept(
	ParseState ps
) {
	size_t len;
	return parse_get(ps, &len);
}

static bool
parse_peek_closing(
	ParseState ps,
	char32_t   e
) {
	char const *cs = parse_peek(ps);
	char32_t    c  = utf8chr(cs, &cs);

	return (c == e);
}

static bool
parse_peek_primary(
	ParseState ps
) {
	char const *cs = parse_peek(ps);
	char32_t    c  = utf8chr(cs, &cs);
	return is_Primary(c);
}

static bool
parse_peek_operate(
	ParseState ps
) {
	char const *cs = parse_peek(ps);
	char32_t    c  = utf8chr(cs, &cs);
	return is_Operator(c);
}

static bool
parse_peek_assemblage(
	ParseState ps
) {
	return *parse_peek(ps) == ';';
}

static bool
parse_peek_sequence(
	ParseState ps
) {
	return *parse_peek(ps) == ',';
}

//------------------------------------------------------------------------------

static Ast
parse_assemblage(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
);

static Ast
parse_primary(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
) {
	Ast expr = ZEN;

	size_t      len;
	char const *leme = parse_peek(ps);
	sloc_t      sloc = parse_sloc(ps);

	switch(*leme) {
	case '(':
		if(ps->len > 1) {
			leme = parse_get(ps, &len);
			break;
		}
		parse_accept(ps);
		if(!parse_peek_closing(ps, ')')) {
			expr = parse_assemblage(ps, ast);
			if(parse_peek_closing(ps, ')')) {
				parse_accept(ps);
			}
			return expr;
		}
		if(parse_peek_closing(ps, ')')) {
			parse_accept(ps);
		}
		leme = "()";
		len  = 2;
		break;
	case '[':
		if(ps->len > 1) {
			leme = parse_get(ps, &len);
			break;
		}
		parse_accept(ps);
		if(!parse_peek_closing(ps, ']')) {
			expr = parse_assemblage(ps, ast);
		}
		if(parse_peek_closing(ps, ']')) {
			parse_accept(ps);
		}
		leme = "[]";
		len  = 2;
		break;
	case '{':
		if(ps->len > 1) {
			leme = parse_get(ps, &len);
			break;
		}
		parse_accept(ps);
		if(!parse_peek_closing(ps, '}')) {
			expr = parse_assemblage(ps, ast);
		}
		if(parse_peek_closing(ps, '}')) {
			parse_accept(ps);
		}
		leme = "{}";
		len  = 2;
		break;
	case ')': case ']': case '}':
	case ';': case ',':
	case '\0':
		return ZEN;

	default:
		leme = parse_get(ps, &len);
		break;
	}

	return ast(sloc, leme, len, ZEN, expr);
}

static Ast
parse_applicate(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
) {
	Ast expr = parse_primary(ps, ast);

	while(parse_peek_primary(ps)) {
		Ast rexpr = parse_primary(ps, ast);
		expr      = ast(rexpr->sloc, "", 0, expr, rexpr);
	}

	return expr;
}

static Ast
parse_operation_1(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	),
	Precedence prec
) {
	Ast expr = (prec < P_Binding) ? (
		parse_operation_1(ps, ast, prec+1)
	) : (
		parse_applicate(ps, ast)
	);

	while(parse_peek_operate(ps) && (precedence(ps->leme, ps->len) == prec)) {
		size_t      len;
		sloc_t      sloc  = parse_sloc(ps);
 		char const *op    = parse_get(ps, &len);
		Ast         rexpr = (prec < P_Binding) ? (
			parse_operation_1(ps, ast, prec+1)
		) : (
			parse_applicate(ps, ast)
		);

		expr = ast(sloc, op, len, expr, rexpr);
	}

	return expr;
}

static Ast
parse_operation(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
) {
	return parse_operation_1(ps, ast, P_Declarative);
}

static Ast
parse_sequence(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
) {
	Ast expr = parse_operation(ps, ast);

	for(Ast last = expr, *lastp = &expr;
		parse_peek_sequence(ps);
		*lastp = last, lastp = &last->m.rexpr
	) {
		size_t      len;
		sloc_t      sloc  = parse_sloc(ps);
		char const *seq   = parse_get(ps, &len);
		Ast         rexpr = parse_operation(ps, ast);

		last = ast(sloc, seq, len, *lastp, rexpr);
	}

	return expr;
}

static Ast
parse_assemblage(
	ParseState ps,
	Ast      (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	)
) {
	Ast expr = parse_sequence(ps, ast);

	for(Ast last = expr, *lastp = &expr;
		parse_peek_assemblage(ps);
		*lastp = last, lastp = &last->m.rexpr
	) {
		size_t      len;
		sloc_t      sloc  = parse_sloc(ps);
		char const *seq   = parse_get(ps, &len);
		Ast         rexpr = parse_sequence(ps, ast);

		last = ast(sloc, seq, len, *lastp, rexpr);
	}

	return expr;
}

//------------------------------------------------------------------------------

Ast
parse(
	char const   *cs,
	char const  **endp,
	unsigned      source,
	unsigned     *linop,
	Ast         (*ast)(
		sloc_t      sloc,
		char const *leme,
		size_t      len,
		...
	),
	bool          all
) {
	struct parse_state ps = {
		NULL, 0,
		cs,
		source,
		cs,
		linop
	};

	Ast expr = all ? (
		parse_assemblage(&ps, ast)
	) : (
		parse_sequence(&ps, ast)
	);

	*endp = ps.cs;

	if(parse_peek_assemblage(&ps)) {
		parse_accept(&ps);
	}

	return expr;
}

