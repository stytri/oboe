#ifndef ENV_H_INCLUDED
#define ENV_H_INCLUDED
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

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern Ast
new_env(
	sloc_t sloc,
	Ast    outer
);

extern Array
dup_env(
	sloc_t sloc,
	Array  env
);

extern Ast
link_env(
	sloc_t sloc,
	Ast    ast,
	Ast    outer
);

//------------------------------------------------------------------------------

extern size_t
locate(
	Ast         env,
	uint64_t    hash,
	char const *leme,
	size_t      len
);

extern Ast
lookup(
	Ast         env,
	uint64_t    hash,
	char const *leme,
	size_t      len,
	size_t      depth
);

extern size_t
define(
	Ast      env,
	uint64_t hash,
	Ast      def,
	Attr     attr
);

//------------------------------------------------------------------------------

extern size_t
atenv(
	Ast env,
	Ast ident
);

extern Ast
inenv(
	Ast env,
	Ast ident
);

extern Ast
named_inenv(
	Ast         env,
	sloc_t      sloc,
	char const *name
);

extern Ast
addenv(
	Ast    env,
	sloc_t sloc,
	Ast    ident,
	Ast    def,
	Attr   attr
);

extern Ast
addenv_args(
	Ast    to,
	Ast    env,
	sloc_t sloc,
	Ast    idents,
	Ast    args
);

extern Ast
addenv_operands(
	Ast    to,
	Ast    env,
	sloc_t sloc,
	Ast    idents,
	Ast    lexpr,
	Ast    rexpr
);

extern void
addenv_argv(
	Ast    to,
	sloc_t sloc,
	int    argc,
	char  *argv[]
);

extern void
addenv_named(
	Ast         to,
	sloc_t      sloc,
	char const *name,
	Ast         ast,
	Attr        attr
);

//------------------------------------------------------------------------------

extern Ast operators;
extern Ast globals;

int
initialise_env(
	void
);

//------------------------------------------------------------------------------

extern Ast
getopr(
	size_t index
);

extern char const *
getops(
	size_t index
);

//------------------------------------------------------------------------------

extern void
env_gc_mark(
	void const *p,
	void      (*mark)(void const *)
);

extern void
env_gc_sweep(
	void const *p
);

struct env_stats {
	size_t live;
	size_t dead;
	size_t total;
};
#define ENV_STATS(...)  (struct env_stats){ 0, 0, 0 }

extern struct env_stats env_stats;

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef DEFTABLE_H_INCLUDED
