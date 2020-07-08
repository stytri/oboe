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
#include "graph.h"
#include "strlib.h"
#include "env.h"
#include "odt.h"

//------------------------------------------------------------------------------

static size_t
subgraph(
	FILE   *stream,
	Ast     expr,
	size_t *idp,
	size_t  cid
);

//------------------------------------------------------------------------------

static void
escaped(
	FILE       *stream,
	char const *cs
) {
	if(cs && *cs) {
		String s = EscapeString(NULL, cs, (char **)&cs, "\\|{}[]<>\"", '\0');
		if(s) {
			fputs(StringToCharLiteral(s, NULL), stream);

			StringDelete(s);
		}
	}
}

static size_t
graph_integer(
	FILE       *stream,
	char const *prefix,
	uint64_t    value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	fprintf(stream, "%"PRIu64, value);
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_float(
	FILE       *stream,
	char const *prefix,
	double      value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	fprintf(stream, "%g", value);
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_character(
	FILE       *stream,
	char const *prefix,
	uint64_t    value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	char cs[2] = { (char)value, '\0' };

	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	escaped(stream, "'");
	escaped(stream, cs);
	escaped(stream, "'");
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_string(
	FILE       *stream,
	char const *prefix,
	String      value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	escaped(stream, "\"");
	escaped(stream, StringToCharLiteral(value, NULL));
	escaped(stream, "\"");
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_identifier(
	FILE       *stream,
	char const *prefix,
	String      value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	fprintf(stream, "%s", StringToCharLiteral(value, NULL));
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_lexeme(
	FILE       *stream,
	char const *prefix,
	char const *value,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,label=\"", id, cid);
	escaped(stream, prefix);
	fprintf(stream, "%s", value);
	escaped(stream, postfix);
	fprintf(stream, "\"];\n");

	return id;
	(void)idp;
}

static size_t
graph_expression(
	FILE       *stream,
	char const *prefix,
	Ast         expr,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,shape=record,label=\"{", id, cid);
	escaped(stream, prefix);
	escaped(stream, postfix);
	fprintf(stream, "|<expr>}\"];\n");
	if(expr) {
		size_t eid = subgraph(stream, expr, idp, cid);
		fprintf(stream, "\t\tnode%zu:expr -> node%zu [headport=n];\n", id, eid);
	}
	return id;
}

static size_t
graph_operator(
	FILE       *stream,
	char const *prefix,
	Ast         lexpr,
	char const *lexeme,
	Ast         rexpr,
	char const *postfix,
	size_t     *idp,
	size_t      cid,
	size_t      id
) {
	fprintf(stream, "\t\tnode%zu [group=cluster%zu,shape=record,label=\"{", id, cid);
	escaped(stream, prefix);
	escaped(stream, lexeme);
	escaped(stream, postfix);
	fprintf(stream, "%s{<lexpr>|<rexpr>}}\"];\n", (lexeme && *lexeme) ? "|" : "");

	size_t lid = subgraph(stream, lexpr, idp, cid);
	fprintf(stream, "\t\tnode%zu:lexpr -> node%zu [headport=n];\n", id, lid);

	size_t rid = subgraph(stream, rexpr, idp, cid);
	fprintf(stream, "\t\tnode%zu:rexpr -> node%zu [headport=n];\n", id, rid);

	return id;
}

static size_t
subgraph(
	FILE   *stream,
	Ast     ast,
	size_t *idp,
	size_t  cid
) {
	size_t id = ++*idp;

	switch(ast_type(ast)) {
	default: {
#	define ENUM(Name) } break; case AST_##Name: {
#	define STR(...)  __VA_ARGS__;

#	define INTEGER(Pre,Val,Post)               return graph_integer   (stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define FLOAT(Pre,Val,Post)                 return graph_float     (stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define CHARACTER(Pre,Val,Post)             return graph_character (stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define STRING(Pre,Val,Post)                return graph_string    (stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define IDENTIFIER(Pre,Val,Post)            return graph_identifier(stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define LEXEME(Pre,Val,Post)                return graph_lexeme    (stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define OPERATOR(Pre,Lexpr,Val,Rexpr,Post)  return graph_operator  (stream, (Pre), (Lexpr), (Val), (Rexpr), (Post), idp, cid, id)
#	define SEQUENCE(Pre,Lexpr,Val,Rexpr,Post)  return graph_operator  (stream, (Pre), (Lexpr), (Val), (Rexpr), (Post), idp, cid, id)
#	define EXPRESSION(Pre,Val,Post)            return graph_expression(stream, (Pre),          (Val),          (Post), idp, cid, id)
#	define ENVIRONMENT(Pre,Array,Post)         return graph_expression(stream, (Pre),           NULL,          (Post), idp, cid, id)
#	define ERRORMESSAGE(Pre,Val,Post)          return graph_lexeme    (stream, (Pre),  oboerrs((Val)->qual),   (Post), idp, cid, id)
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

	return id;
}

//------------------------------------------------------------------------------

void
graph_header(
	FILE *stream
) {
	fputs("digraph g {\n", stream);
	fputs("\tcompound=true;\n", stream);
	fputs("\tsplines=false;\n", stream);
}

void
graph_footer(
	FILE *stream
) {
	fputs("}\n", stream);
}

void
graph(
	FILE       *stream,
	char const *title,
	Ast         ast
) {
	static size_t uid = 0;

	size_t cid = ++uid;

	fprintf(stream, "\tsubgraph cluster%zu {\n", cid);

	if(title && *title) {
		fprintf(stream, "\t\tlabel=\"");
		escaped(stream, title);
		fprintf(stream, "\";\n");
	}

	subgraph(stream, ast, &uid, cid);

	fprintf(stream, "\t}\n");
}

