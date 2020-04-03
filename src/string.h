#ifndef STRING_H_INCLUDED
#define STRING_H_INCLUDED
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

#include <string.h>
#include "stdtypes.h"
#include "bitmac.h"
#include "xmem.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef struct string *String;

enum { STRING_MAX = BIT_ROUND(XMEM_MAX/2) };
enum { STRING_MIN = XMEM_MIN };

//------------------------------------------------------------------------------

extern size_t
StringLength(
	String s
);

extern size_t
StringCapacity(
	String s
);

extern int
StringGetChar(
	String s,
	size_t i
);

extern String
NullString(
	void
);

extern String
StringCreate(
	void
);

extern String
StringReserve(
	size_t n
);

extern String
StringBuild(
	int  (*get)(
		void *context
	),
	void  *context,
	size_t reserve
);

extern void
StringDelete(
	String s
);

extern String
DuplicateString(
	String s
);

extern String
RepeatedString(
	String s,
	size_t n
);

extern String
CharToString(
	int c
);

extern String
RepeatedCharToString(
	int    c,
	size_t n
);

extern String
CharsToString(
	int c1,
	int c2
);

extern String
CharLiteralToString(
	char const cs[],
	size_t     n
);

extern String
CharLiteralsToString(
	char const cs1[],
	size_t     n1,
	char const cs2[],
	size_t     n2
);

extern String
StringAppendChar(
	String t,
	int    c
);

extern String
StringAppendCharLiteral(
	String     t,
	char const cs[],
	size_t     n
);

extern String
StringAppend(
	String t,
	String s
);

extern String
StringConcatenate(
	String s1,
	String s2
);

extern String
StringPrefix(
	String s,
	size_t n
);

extern String
StringSuffix(
	String s,
	size_t n
);

extern String
SubString(
	String s,
	size_t o,
	size_t n
);

extern char const *
StringToCharLiteral(
	String  s,
	size_t *n
);

extern int
StringEqualCharLiteral(
	String      s,
	char const *cs,
	size_t      n
);
static inline int
StringNotEqualCharLiteral(
	String      s,
	char const *cs,
	size_t      n
) {
	return !StringEqualCharLiteral(s, cs, n);
}

extern int
StringEqual(
	String s1,
	String s2
);
static inline int
StringNotEqual(
	String s1,
	String s2
) {
	return !StringEqual(s1, s2);
}

extern int
StringCompare(
	String s1,
	String s2
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef STRING_H_INCLUDED
