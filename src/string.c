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
#include "string.h"
#include "bits.h"

//------------------------------------------------------------------------------

struct string {
	size_t len;
	size_t cap;
	char   cs[];
};

//------------------------------------------------------------------------------

static size_t const  sizes[] = {
	(STRING_MIN * 1),
	(STRING_MIN * 2),
	(STRING_MIN * 3),
	(STRING_MIN * 4),
	(STRING_MIN * 5),
	(STRING_MIN * 6),
	(STRING_MIN * 7),
	(STRING_MIN * 8 * 1),
	(STRING_MIN * 8 * 2),
	(STRING_MIN * 8 * 3),
	(STRING_MIN * 8 * 4),
	(STRING_MIN * 8 * 5),
	(STRING_MIN * 8 * 6),
	(STRING_MIN * 8 * 7),
	(STRING_MIN * 8 * 8 * 1),
	(STRING_MIN * 8 * 8 * 2),
	(STRING_MIN * 8 * 8 * 3),
	(STRING_MIN * 8 * 8 * 4),
	(STRING_MIN * 8 * 8 * 5),
	(STRING_MIN * 8 * 8 * 6),
	(STRING_MIN * 8 * 8 * 7),
	(STRING_MIN * 8 * 8 * 8 *  1),
	(STRING_MIN * 8 * 8 * 8 *  2),
	(STRING_MIN * 8 * 8 * 8 *  3),
	(STRING_MIN * 8 * 8 * 8 *  4),
	(STRING_MIN * 8 * 8 * 8 *  5),
	(STRING_MIN * 8 * 8 * 8 *  6),
	(STRING_MIN * 8 * 8 * 8 *  7),
	(STRING_MIN * 8 * 8 * 8 *  8),
	(STRING_MIN * 8 * 8 * 8 *  9),
	(STRING_MIN * 8 * 8 * 8 * 10),
	(STRING_MIN * 8 * 8 * 8 * 11),
	(STRING_MIN * 8 * 8 * 8 * 12),
	(STRING_MIN * 8 * 8 * 8 * 13),
	(STRING_MIN * 8 * 8 * 8 * 14),
	(STRING_MIN * 8 * 8 * 8 * 15),
	(STRING_MIN * 8 * 8 * 8 * 16 *  1),
	(STRING_MIN * 8 * 8 * 8 * 16 *  2),
	(STRING_MIN * 8 * 8 * 8 * 16 *  3),
	(STRING_MIN * 8 * 8 * 8 * 16 *  4),
	(STRING_MIN * 8 * 8 * 8 * 16 *  5),
	(STRING_MIN * 8 * 8 * 8 * 16 *  6),
	(STRING_MIN * 8 * 8 * 8 * 16 *  7),
	(STRING_MIN * 8 * 8 * 8 * 16 *  8),
	(STRING_MIN * 8 * 8 * 8 * 16 *  9),
	(STRING_MIN * 8 * 8 * 8 * 16 * 10),
	(STRING_MIN * 8 * 8 * 8 * 16 * 11),
	(STRING_MIN * 8 * 8 * 8 * 16 * 12),
	(STRING_MIN * 8 * 8 * 8 * 16 * 13),
	(STRING_MIN * 8 * 8 * 8 * 16 * 14),
	(STRING_MIN * 8 * 8 * 8 * 16 * 15),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  1),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  2),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  3),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  4),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  5),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  6),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  7),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  8),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 *  9),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 10),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 11),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 12),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 13),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 14),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 15),
	(STRING_MIN * 8 * 8 * 8 * 16 * 16 * 16),
};
static size_t const n_sizes = sizeof(sizes) / sizeof(sizes[0]);

static size_t
string_allocation_size(
	size_t siz
) {
	for(size_t i = 0; i < n_sizes; ++i) {
		if(siz < sizes[i]) {
			return sizes[i];
		}
	}

	siz = capacity_of(siz + 1);
	if(siz < STRING_MAX) {
		return siz;
	}

	return STRING_MAX;
}

static inline char const *
char_pointer(
	StringConst s
) {
	return s ? s->cs : "";
}

static inline size_t
char_length(
	char const *cs,
	size_t      n
) {
	return cs ? n : 0;
}

static inline String
expand_string(
	String s,
	size_t n
) {
	size_t o = s->len;
	size_t m = s->cap;

	if(n > (m - o)) {
		if((n > (STRING_MAX - o)) || (m >= STRING_MAX)) {
			return NULL;
		}

		size_t z = string_allocation_size(o + n);
		s = xrealloc(s, sizeof(*s) + z);
		if(s) {
			s->cap = z - 1;
		}
	}

	return s;
}

//------------------------------------------------------------------------------

inline size_t
StringLength(
	StringConst s
) {
	return s ? s->len : 0;
}

inline size_t
StringCapacity(
	StringConst s
) {
	return s ? s->cap : 0;
}

int
StringGetChar(
	StringConst s,
	size_t      i
) {
	size_t n = StringLength(s);

	if(i < n) {
		return s->cs[i];
	}

	return -1;
}

StringConst
NullString(
	void
) {
	static StringConst nulls = NULL;

	if(!nulls) {
		nulls = StringCreate();
	}

	return nulls;
}

String
StringCreate(
	void
) {
	size_t z = string_allocation_size(0);
	String s = xmalloc(sizeof(*s) + z);

	if(s) {
		s->len = 0;
		s->cap = z - 1;
		s->cs[0] = '\0';
	}

	return s;
}

String
StringReserve(
	size_t n
) {
	String s = NULL;

	if(n < STRING_MAX) {
		size_t z = string_allocation_size(n);
		s = xmalloc(sizeof(*s) + z);

		if(s) {
			s->len = n;
			s->cap = z - 1;
			s->cs[n] = '\0';
		}
	}

	return s;
}

String
StringBuild(
	int  (*get)(
		void *context
	),
	void  *context,
	size_t reserve
) {
	String s = NULL;

	if(reserve < STRING_MAX) {
		size_t z = string_allocation_size(reserve);
		s = xmalloc(sizeof(*s) + z);
		if(!s) {
			return s;
		}

		s->len = 0;
		s->cap = z - 1;
	}

	for(int c = get(context);
		c > 0;
		c = get(context)
	) {
		if(s->len == s->cap) {
			String t = s;
			s = expand_string(s, s->len);
			if(!s) {
				t->cs[t->len] = '\0';
				return t;
			}
		}

		s->cs[s->len++] = c;
	}

	s->cs[s->len] = '\0';

	size_t z = string_allocation_size(s->len);
	if(s->cap >= z) {
		String t = s;
		s = xrealloc(s, sizeof(*s) + z);
		if(!s) {
			return t;
		}
		s->cap = z - 1;
	}

	return s;
}

void
StringDelete(
	String s
) {
	xfree(s);
}

String
DuplicateString(
	StringConst s
) {
	String t = StringReserve(StringLength(s));
	if(s && t) {
		memcpy(t->cs, s->cs, s->len);
	}

	return t;
}

String
RepeatedString(
	StringConst s,
	size_t      n
) {
	size_t len = StringLength(s);
	if((n > 0)
		 && ((STRING_MAX < n) || (len > (STRING_MAX / n)))
	) {
		return NULL;
	}

	size_t siz = n * len;
	String t   = StringReserve(siz);
	if(t && (siz > 0)) {

		for(char *cs = t->cs; n-- > 0; cs += len) {
			memcpy(cs, s->cs, len);
		}
	}

	return t;
}

String
CharToString(
	int c
) {
	if(c > 0) {
		char const cs[2] = { c, '\0' };

		return CharLiteralToString(cs, 1);
	}

	return CharLiteralToString("", 0);
}

String
RepeatedCharToString(
	int    c,
	size_t n
) {
	String s = StringReserve(n);
	if(s) {
		memset(s->cs, c, n);
	}

	return s;
}

String
CharsToString(
	int c1,
	int c2
) {
	if(c1 > 0) {
		if(c2 > 0) {
			char const cs[3] = { c1, c2, '\0' };

			return CharLiteralToString(cs, 2);
		}

		return CharToString(c1);
	}

	if(c2 > 0) {
		return CharToString(c2);
	}

	return CharLiteralToString("", 0);
}

String
CharLiteralToString(
	char const *cs,
	size_t      n
) {
	n = char_length(cs, n);

	size_t z = string_allocation_size(n);
	String s = xmalloc(sizeof(*s) + z);

	if(s) {
		memcpy(s->cs, cs, n);
		s->len = n;
		s->cap = z - 1;
		s->cs[n] = '\0';
	}

	return s;
}

String
CharLiteralsToString(
	char const cs1[],
	size_t     n1,
	char const cs2[],
	size_t     n2
) {
	size_t n = n1 + n2;

	size_t z = string_allocation_size(n);
	String s = xmalloc(sizeof(*s) + z);

	if(s) {
		if(n1 > 0) {
			memcpy(s->cs, cs1, n1);
		}
		if(n2 > 0) {
			memcpy(s->cs+n1, cs2, n2);
		}
		s->len = n;
		s->cap = z - 1;
		s->cs[n] = '\0';
	}

	return s;
}

String
StringAppendChar(
	String t,
	int    c
) {
	if(t && (c > 0)) {
		if(t->len == t->cap) {
			t = expand_string(t, 1);
			if(!t) {
				return t;
			}
		}

		t->cs[t->len++] = c;
		t->cs[t->len]   = '\0';
	}

	return t;
}

String
StringAppendCharLiteral(
	String     t,
	char const cs[],
	size_t     n
) {
	if(t && (n > 0)) {
		t = expand_string(t, n);

		if(t) {
			memcpy(t->cs + t->len, cs, n);
			t->len       += n;
			t->cs[t->len] = '\0';
		}
	}

	return t;
}

String
StringAppend(
	String      t,
	StringConst s
) {
	return StringAppendCharLiteral(t, char_pointer(s), StringLength(s));
}

String
StringConcatenate(
	StringConst s1,
	StringConst s2
) {
	size_t n1 = StringLength(s1);
	size_t n2 = StringLength(s2);
	size_t n  = n1 + n2;
	size_t z  = string_allocation_size(n);
	String s  = xmalloc(sizeof(*s) + z);

	if(s) {
		memcpy(s->cs, char_pointer(s1), n1);
		memcpy(s->cs + n1, char_pointer(s2), n2);
		s->len = n;
		s->cap = z - 1;
		s->cs[n] = '\0';
	}

	return s;
}

String
StringPrefix(
	StringConst s,
	size_t      n
) {
	return SubString(s, 0, n);
}

String
StringSuffix(
	StringConst s,
	size_t      n
) {
	size_t sn = StringLength(s);
	if(n > sn) {
		n = sn;
	}
	return SubString(s, sn - n, n);
}

String
SubString(
	StringConst s,
	size_t      o,
	size_t      n
) {
	size_t sn = StringLength(s);
	if((sn >= n) && ((sn - n) >= o)) {
		return CharLiteralToString(char_pointer(s) + o, n);
	}

	return CharToString('\0');
}

char const *
StringToCharLiteral(
	StringConst s,
	size_t     *n
) {
	if(n) {
		*n = StringLength(s);
	}
	return char_pointer(s);
}

int
StringEqualCharLiteral(
	StringConst s,
	char const *cs,
	size_t      n
) {
	n = char_length(cs, n);
	if(n == StringLength(s)) {
		return memcmp(cs, char_pointer(s), n) == 0;
	}

	return 0;
}

int
StringEqual(
	StringConst s1,
	StringConst s2
) {
	size_t n = StringLength(s1);
	if(n == StringLength(s2)) {
		return memcmp(char_pointer(s1), char_pointer(s2), n) == 0;
	}

	return 0;
}

int
StringCompare(
	StringConst s1,
	StringConst s2
) {
	size_t      n1, n2;
	char const *cs1 = StringToCharLiteral(s1, &n1);
	char const *cs2 = StringToCharLiteral(s2, &n2);

	if(n1 < n2) {
		int r = strncmp(cs1, cs2, n1);
		if(r == 0) {
			return -1;
		}
		return r;
	}

	int r = strncmp(cs1, cs2, n2);
	if((r == 0) && (n1 > n2)) {
		return 1;
	}
	return r;
}

