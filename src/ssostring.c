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
#include "string.h"
#include "bits.h"
#include "gc.h"

//------------------------------------------------------------------------------

#define SIZE_BIT (CHAR_BIT * sizeof(size_t))

struct string {
	size_t len;
	size_t cap;
	char  *ptr;
};

#define SSO_SIZE (sizeof(struct string) - 1)
#define SSO_MASK ((1 << (CHAR_BIT - 1)) - 1)

//------------------------------------------------------------------------------

static inline bool
is_little_endian(
	void
) {
	return *(char*)((int[]){1});
}

static inline size_t
sso_bit(
	void
) {
	return is_little_endian() ? 1 : ~(SIZE_C(~0) >> 1);
}

static inline size_t
sso_shift(
	void
) {
	return is_little_endian() ? 1 : (SIZE_BIT - CHAR_BIT);
}

static inline bool
is_sso_string(
	StringConst s
) {
	return (s->len & sso_bit()) != 0;
}

static inline size_t
sso_string_len(
	StringConst s
) {
	return (s->len >> sso_shift()) & SSO_MASK;
}

static inline size_t
sso_string_cap(
	StringConst s
) {
	return SSO_SIZE - 1;
	(void)s;
}

static inline char *
sso_string_ptr(
	StringConst s
) {
	return ((char *)&(s->len)) + 1;
}

static inline size_t
string_len(
	StringConst s
) {
	return s->len >> is_little_endian();
}

static inline size_t
string_cap(
	StringConst s
) {
	return s->cap;
}

static inline char *
string_ptr(
	StringConst s
) {
	return s->ptr;
}

static inline size_t
string_length(
	StringConst s
) {
	return is_sso_string(s) ? sso_string_len(s) : string_len(s);
}

static inline size_t
string_capacity(
	StringConst s
) {
	return is_sso_string(s) ? sso_string_cap(s) : string_cap(s);
}

static inline char *
string_pointer(
	StringConst s
) {
	return is_sso_string(s) ? sso_string_ptr(s) : string_ptr(s);
}

static inline void
unpack_string(
	StringConst s,
	String      t
) {
	t->len = string_length(s);
	t->cap = string_capacity(s);
	t->ptr = string_pointer(s);
}

static inline String
pack_string(
	String      s,
	StringConst t
) {
	if(t->len >= SSO_SIZE) {
		s->len = t->len << is_little_endian();
		s->cap = t->cap;
		s->ptr = t->ptr;

	} else if(sso_string_ptr(s) == t->ptr) {
		struct string st;
		memset(&st, 0, sizeof(st));
		st.len = (t->len << sso_shift()) | sso_bit();
		memcpy(sso_string_ptr(&st), t->ptr, t->len);
		memcpy(s, &st, sizeof(st));

	} else {
		char *st = sso_string_ptr(s);
		s->len = (t->len << sso_shift()) | sso_bit();
		memcpy(st, t->ptr, t->len);
		memset(st + t->len, 0, (SSO_SIZE - t->len));
	}

	return s;
}

//------------------------------------------------------------------------------

static size_t const  sizes[] = {
#if 0
	(STRING_MIN * 1),
	(STRING_MIN * 2),
	(STRING_MIN * 3),
	(STRING_MIN * 4),
	(STRING_MIN * 5),
	(STRING_MIN * 6),
	(STRING_MIN * 7),
#endif
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
	if(siz >= SSO_SIZE) {
		if(siz < ((STRING_MIN * 8) - 1)) {
			return ((siz + 1) + (STRING_MIN - 1)) & ~(STRING_MIN - 1);
		}

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

	return SSO_SIZE;
}

static inline char const *
char_pointer(
	StringConst s
) {
	return s ? string_pointer(s) : "";
}

static inline size_t
char_length(
	char const *cs,
	size_t      n
) {
	return cs ? n : 0;
}

static bool
expand_unpacked_string(
	String t,
	size_t n
) {
	size_t const o = t->len;
	size_t const m = t->cap;

	if(n > (m - o)) {
		if((n > (STRING_MAX - o)) || (m >= STRING_MAX)) {
			return false;
		}

		size_t const z = string_allocation_size(o + n);
		size_t const w = z - 1;
		if(w > m) {

			if(w >= SSO_SIZE) {
				if(m >= SSO_SIZE) {

					void *p = realloc(t->ptr, z);
					if(!p) {
						return false;
					}
					t->ptr = p;

				} else {

					void *p = malloc(z);
					if(!p) {
						return false;
					}
					t->ptr = memcpy(p, t->ptr, o+1);
				}
			}

			t->cap = w;
		}
	}

	return true;
}

static String
make_string(
	size_t reserve,
	size_t length
) {
	if(reserve < STRING_MAX) {
		static char nuls[SSO_SIZE + 1] = { 0 };

		reserve = string_allocation_size(reserve);
		char *p = (reserve >= SSO_SIZE) ? calloc(1, reserve) : nuls;
		if(p) {

			struct string const t = { length, reserve - !reserve, p };
			String              s = malloc(sizeof(*s));
			if(s) {
				return pack_string(s, &t);
			}
		}
	}

	return NULL;
}

//------------------------------------------------------------------------------

inline size_t
StringLength(
	StringConst s
) {
	return s ? string_length(s) : 0;
}

inline size_t
StringCapacity(
	StringConst s
) {
	return s ? string_capacity(s) : 0;
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
	return make_string(0, 0);
}

String
StringReserve(
	size_t n
) {
	return make_string(n, n);
}

String
StringBuild(
	int  (*get)(
		void *context
	),
	void  *context,
	size_t reserve
) {
	String s = make_string(reserve, 0);
	if(!s) {
		return s;
	}

	struct string t;
	unpack_string(s, &t);

	for(int c = get(context);
		c > 0;
		c = get(context)
	) {
		if(t.len == t.cap) {
			if(!expand_unpacked_string(&t, t.len)) {
				break;
			}
		}

		t.ptr[t.len++] = c;
	}

	t.ptr[t.len] = '\0';

	return pack_string(s, &t);
}

void
StringDelete(
	StringConst s
) {
	if(s) {
		if(!is_sso_string(s)) {
			free(s->ptr);
		}
		free(s);
	}
}

String
DuplicateString(
	StringConst s
) {
	size_t n = StringLength(s);
	String t = StringReserve(n);
	if(s && t) {
		memcpy(string_pointer(t), string_pointer(s), n);
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

		char const *cs = string_pointer(s);
		for(char   *tp = string_pointer(t); n-- > 0; tp += len) {
			memcpy(tp, cs, len);
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
		memset(string_pointer(s), c, n);
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

	String s = StringReserve(n);
	if(s) {
		memcpy(string_pointer(s), cs, n);
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

	String s = StringReserve(n);
	if(s) {
		char *sp = string_pointer(s);

		if(n1 > 0) {
			memcpy(sp, cs1, n1);
		}
		if(n2 > 0) {
			memcpy(sp+n1, cs2, n2);
		}
	}

	return s;
}

String
StringAppendChar(
	String s,
	int    c
) {
	if(s && (c > 0)) {
		struct string t;
		unpack_string(s, &t);

		if(expand_unpacked_string(&t, 1)) {
			t.ptr[t.len++] = c;
			t.ptr[t.len]   = '\0';

			return pack_string(s, &t);
		}
	}

	return s;
}

String
StringAppendCharLiteral(
	String     s,
	char const cs[],
	size_t     n
) {
	if(s && (n > 0)) {
		struct string t;
		unpack_string(s, &t);

		if(expand_unpacked_string(&t, n)) {
			memcpy(t.ptr + t.len, cs, n);
			t.len        += n;
			t.ptr[t.len]  = '\0';

			return pack_string(s, &t);
		}
	}

	return s;
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
	return CharLiteralsToString(
		char_pointer(s1), StringLength(s1),
		char_pointer(s2), StringLength(s2)
	);
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

