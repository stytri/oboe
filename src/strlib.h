#ifndef STRLIB_H_INCLUDED
#define STRLIB_H_INCLUDED
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

#include "stdtypes.h"
#include "string.h"
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

static inline int
todigit(
	int c
) {
	return c - '0';
}

static inline int
toxdigit(
	int c
) {
	return isalpha(c) ? (10 + (toupper(c) - 'A')) : (c - '0');
}

//------------------------------------------------------------------------------

extern bool
is_float(
	char const *leme,
	size_t      len
);

//------------------------------------------------------------------------------

static inline bool
streq(
	char const *cs,
	char const *ct
) {
	return (strcmp(cs, ct) == 0);
}

extern int
strtoc(
	char const *cs,
	char      **endp
);

extern uintmax_t
strtou(
	char const *cs,
	char      **endp
);

//------------------------------------------------------------------------------

extern String
UnEscapeString(
	String      t,
	char const *cs,
	char      **endp,
	int  const  q
);

extern String
EscapeString(
	String      t,
	char const *cs,
	char      **endp,
	char const *esc,
	int  const  q
);

extern uint64_t
HashString(
	String s
);

//------------------------------------------------------------------------------

extern size_t
codepointoffset(
	char   const *cs,
	size_t const  n,
	size_t        r
);

extern size_t
reversecodepointoffset(
	char   const *cs,
	size_t const  n,
	size_t        r
);

//------------------------------------------------------------------------------

extern int
is_absolutepath(
	char const *cs
);

extern int
is_relativepath(
	char const *cs
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef STRLIB_H_INCLUDED
