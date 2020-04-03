#ifndef UCS_CTYPE_H_INCLUDED
#define UCS_CTYPE_H_INCLUDED
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

//------------------------------------------------------------------------------

#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

enum {
	UCS_Letter_uppercase = UINT32_C(0x1),
	UCS_Letter_lowercase = UINT32_C(0x2),
	UCS_Letter_titlecase = UINT32_C(0x4),
	UCS_Letter_cased = UINT32_C(0x7),
	UCS_Letter_modifier = UINT32_C(0x8),
	UCS_Letter_other = UINT32_C(0x10),
	UCS_Letter = UINT32_C(0x1f),
	UCS_Mark_nonspacing = UINT32_C(0x20),
	UCS_Mark_spacing = UINT32_C(0x40),
	UCS_Mark_enclosing = UINT32_C(0x80),
	UCS_Mark = UINT32_C(0xe0),
	UCS_Number_decimal = UINT32_C(0x100),
	UCS_Number_letter = UINT32_C(0x200),
	UCS_Number_other = UINT32_C(0x400),
	UCS_Number = UINT32_C(0x700),
	UCS_Punctuation_connector = UINT32_C(0x800),
	UCS_Punctuation_dash = UINT32_C(0x1000),
	UCS_Punctuation_open = UINT32_C(0x2000),
	UCS_Punctuation_close = UINT32_C(0x4000),
	UCS_Punctuation_initial = UINT32_C(0x8000),
	UCS_Punctuation_final = UINT32_C(0x10000),
	UCS_Punctuation_other = UINT32_C(0x20000),
	UCS_Punctuation = UINT32_C(0x3f800),
	UCS_Symbol_math = UINT32_C(0x40000),
	UCS_Symbol_currency = UINT32_C(0x80000),
	UCS_Symbol_modifier = UINT32_C(0x100000),
	UCS_Symbol_other = UINT32_C(0x200000),
	UCS_Symbol = UINT32_C(0x3c0000),
	UCS_Separator_space = UINT32_C(0x400000),
	UCS_Separator_line = UINT32_C(0x800000),
	UCS_Separator_paragraph = UINT32_C(0x1000000),
	UCS_Separator = UINT32_C(0x1c00000),

	UCS_Lu = UCS_Letter_uppercase,
	UCS_Ll = UCS_Letter_lowercase,
	UCS_Lt = UCS_Letter_titlecase,
	UCS_Lc = UCS_Letter_cased,
	UCS_Lm = UCS_Letter_modifier,
	UCS_Lo = UCS_Letter_other,
	UCS_L = UCS_Letter,
	UCS_Mn = UCS_Mark_nonspacing,
	UCS_Mc = UCS_Mark_spacing,
	UCS_Me = UCS_Mark_enclosing,
	UCS_M = UCS_Mark,
	UCS_Nd = UCS_Number_decimal,
	UCS_Nl = UCS_Number_letter,
	UCS_No = UCS_Number_other,
	UCS_N = UCS_Number,
	UCS_Pc = UCS_Punctuation_connector,
	UCS_Pd = UCS_Punctuation_dash,
	UCS_Ps = UCS_Punctuation_open,
	UCS_Pe = UCS_Punctuation_close,
	UCS_Pi = UCS_Punctuation_initial,
	UCS_Pf = UCS_Punctuation_final,
	UCS_Po = UCS_Punctuation_other,
	UCS_P = UCS_Punctuation,
	UCS_Sm = UCS_Symbol_math,
	UCS_Sc = UCS_Symbol_currency,
	UCS_Sk = UCS_Symbol_modifier,
	UCS_So = UCS_Symbol_other,
	UCS_S = UCS_Symbol,
	UCS_Zs = UCS_Separator_space,
	UCS_Zl = UCS_Separator_line,
	UCS_Zp = UCS_Separator_paragraph,
	UCS_Z = UCS_Separator,
};
typedef uint32_t UCS_t;

extern int
is_UCS(
	UCS_t    test,
	char32_t c
);

static inline int
is_UCS_Letter_uppercase(
	char32_t c
) {
	return is_UCS(UCS_Letter_uppercase, c);
}
static inline int
is_UCS_Letter_lowercase(
	char32_t c
) {
	return is_UCS(UCS_Letter_lowercase, c);
}
static inline int
is_UCS_Letter_titlecase(
	char32_t c
) {
	return is_UCS(UCS_Letter_titlecase, c);
}
static inline int
is_UCS_Letter_cased(
	char32_t c
) {
	return is_UCS(UCS_Letter_cased, c);
}
static inline int
is_UCS_Letter_modifier(
	char32_t c
) {
	return is_UCS(UCS_Letter_modifier, c);
}
static inline int
is_UCS_Letter_other(
	char32_t c
) {
	return is_UCS(UCS_Letter_other, c);
}
static inline int
is_UCS_Letter(
	char32_t c
) {
	return is_UCS(UCS_Letter, c);
}
static inline int
is_UCS_Mark_nonspacing(
	char32_t c
) {
	return is_UCS(UCS_Mark_nonspacing, c);
}
static inline int
is_UCS_Mark_spacing(
	char32_t c
) {
	return is_UCS(UCS_Mark_spacing, c);
}
static inline int
is_UCS_Mark_enclosing(
	char32_t c
) {
	return is_UCS(UCS_Mark_enclosing, c);
}
static inline int
is_UCS_Mark(
	char32_t c
) {
	return is_UCS(UCS_Mark, c);
}
static inline int
is_UCS_Number_decimal(
	char32_t c
) {
	return is_UCS(UCS_Number_decimal, c);
}
static inline int
is_UCS_Number_letter(
	char32_t c
) {
	return is_UCS(UCS_Number_letter, c);
}
static inline int
is_UCS_Number_other(
	char32_t c
) {
	return is_UCS(UCS_Number_other, c);
}
static inline int
is_UCS_Number(
	char32_t c
) {
	return is_UCS(UCS_Number, c);
}
static inline int
is_UCS_Punctuation_connector(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_connector, c);
}
static inline int
is_UCS_Punctuation_dash(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_dash, c);
}
static inline int
is_UCS_Punctuation_open(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_open, c);
}
static inline int
is_UCS_Punctuation_close(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_close, c);
}
static inline int
is_UCS_Punctuation_initial(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_initial, c);
}
static inline int
is_UCS_Punctuation_final(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_final, c);
}
static inline int
is_UCS_Punctuation_other(
	char32_t c
) {
	return is_UCS(UCS_Punctuation_other, c);
}
static inline int
is_UCS_Punctuation(
	char32_t c
) {
	return is_UCS(UCS_Punctuation, c);
}
static inline int
is_UCS_Symbol_math(
	char32_t c
) {
	return is_UCS(UCS_Symbol_math, c);
}
static inline int
is_UCS_Symbol_currency(
	char32_t c
) {
	return is_UCS(UCS_Symbol_currency, c);
}
static inline int
is_UCS_Symbol_modifier(
	char32_t c
) {
	return is_UCS(UCS_Symbol_modifier, c);
}
static inline int
is_UCS_Symbol_other(
	char32_t c
) {
	return is_UCS(UCS_Symbol_other, c);
}
static inline int
is_UCS_Symbol(
	char32_t c
) {
	return is_UCS(UCS_Symbol, c);
}
static inline int
is_UCS_Separator_space(
	char32_t c
) {
	return is_UCS(UCS_Separator_space, c);
}
static inline int
is_UCS_Separator_line(
	char32_t c
) {
	return is_UCS(UCS_Separator_line, c);
}
static inline int
is_UCS_Separator_paragraph(
	char32_t c
) {
	return is_UCS(UCS_Separator_paragraph, c);
}
static inline int
is_UCS_Separator(
	char32_t c
) {
	return is_UCS(UCS_Separator, c);
}

//------------------------------------------------------------------------------

extern char32_t
to_Uppercase(
	char32_t c
);

extern char32_t
to_Lowercase(
	char32_t c
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef UCS_CTYPE_H_INCLUDED

