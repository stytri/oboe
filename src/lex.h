#ifndef LEX_H_INCLUDED
#define LEX_H_INCLUDED
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

#include "ucs_ctype.h"
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

enum {
	UCS_Operator    = UCS_Sm|UCS_Sk|(UCS_P&~UCS_Pc),

	UCS_ID_Start    = UCS_L |UCS_Nl|UCS_Sc|UCS_Pc,
	UCS_ID_Continue = UCS_ID_Start |UCS_Mn|UCS_Mc|UCS_Nd,
};

//------------------------------------------------------------------------------

static inline int
is_Space(
	char32_t c
) {
	return ('\t' == c) || is_UCS_Separator_space(c);
}

static inline int
is_Digit(
	char32_t c
) {
	return isdigit(c);
}

static inline int
is_XDigit(
	char32_t c
) {
	return isxdigit(c);
}

static inline int
is_ID_Start(
	char32_t c
) {
	return is_UCS(UCS_ID_Start, c);
}

static inline int
is_ID_Continue(
	char32_t c
) {
	return is_UCS(UCS_ID_Continue, c);
}

static inline int
is_Primary(
	char32_t c
) {
	switch(c) {
	default :
		return is_ID_Start(c) || is_Digit(c);
	case '"': case '\'': case '`':
	case '(': case '[': case '{':
		return 1;
	case '\\':
	case ')': case ']': case '}':
	case ';': case ',':
	case '#':
		return 0;
	}
}

static inline int
is_Operator(
	char32_t c
) {
	switch(c) {
	default :
		return is_UCS(UCS_Operator, c);
	case '\\':
		return 1;
	case '"': case '\'': case '`':
	case '(': case '[': case '{':
	case ')': case ']': case '}':
	case ';': case ',':
	case '#':
		return 0;
	}
}

static inline int
is_Reserved(
	char32_t c
) {
	switch(c) {
	default :
		return 0;
	case '\\':
	case '"': case '\'': case '`':
	case '(': case '[': case '{':
	case ')': case ']': case '}':
	case ';': case ',':
	case '#':
		return 1;
	}
}

static inline int
is_EOL(
	char32_t c
) {
	return (c == '\n')
		|| (c == '\r')
		|| (c == '\f')
		|| (c == '\v')
		|| (c == 0x85)   // NEL
		|| (c == 0x2028) // LS
		|| (c == 0x2029) // PS
	;
}

//------------------------------------------------------------------------------

extern char const *
lex(
	char const  *cs,
	char const **endp,
	char const **linep,
	unsigned    *linop
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef LEX_H_INCLUDED
