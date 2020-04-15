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
#include "strlib.h"
#include "nobreak.h"
#include "xmem.h"
#include "hash.h"
#include "utf8.h"

//------------------------------------------------------------------------------

bool
is_float(
	char const *leme,
	size_t      len
) {
	for(char const *end = leme + len, *at = leme; at < end; ++at) {
		if(*at == '.') {
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------

static int
esctoc(
	char const  *cs,
	char const **endp
) {
	int c = *cs;
	int n = 2;

	switch(c) {
	default :           ++cs; break;
	case 'n': c = '\n'; ++cs; break;
	case 't': c = '\t'; ++cs; break;
	case 'U': case 'u':
		n *= 2;
		nobreak;
	case 'W': case 'w':
		n *= 2;
		nobreak;
	case 'X': case 'x':
		c = 0;
		for(int d = *++cs; (n-- > 0) && isxdigit(d); d = *++cs) {
			d = toxdigit(d);
			c = (c << 4) + d;
		}
		break;
	}

	*endp = (char *)cs;

	return c;
}

//------------------------------------------------------------------------------

int
strtoc(
	char const *cs,
	char      **endp
) {
	int c = *cs;

	if(c == '\\') {
		c = esctoc(cs + 1, &cs);
	}

	if(endp) {
		*endp = (char *)cs;
	}

	return c;
}

//------------------------------------------------------------------------------

uintmax_t
strtou(
	char const *cs,
	char      **endp
) {
	char     *s;
	uintmax_t u = strtoumax(cs, &s, 0);

	if(*s) {
		int      c = *s;
		unsigned e = strtoumax(s+1, &s, 0);

		switch(c) {
		default:
			break;
		case 'E': case 'e':
			if(e < 20) {
				for(; e   > 2; u *= 1000, e -= 3)
					;
				for(; e-- > 0; u *= 10)
					;
			}
			break;
		case 'P': case 'p':
			if(e < 64) {
				u <<= e;
			}
			break;
		}
	}

	if(endp) {
		*endp = s;
	}

	return u;
}

//------------------------------------------------------------------------------

String
UnEscapeString(
	String      t,
	char const *cs,
	char      **endp,
	int  const  q
) {
	if(!t) {
		t = StringCreate();
	}

	if(t) {
		String s;

		for(int c = *cs; c && (c != q); c = *cs) {
			if(c == '\\') {
				char  sc[4], *sp;
				int   cc = esctoc(cs + 1, &cs);
				size_t n = utf8encode(sc, &sp, cc);
				s = StringAppendCharLiteral(t, sc, n);
				if(!s) break;
				t = s;
				continue;
			}

			s = StringAppendChar(t, c);
			if(!s) break;
			t = s;
			++cs;
		}
	}

	if(endp) {
		*endp = (char *)cs;
	}

	return t;
}

String
EscapeString(
	String      t,
	char const *cs,
	char      **endp,
	char const *esc,
	int  const  q
) {
	if(!t) {
		t = StringCreate();
	}

	if(t) {
		String s;

		for(int c = *cs; c && (c != q); c = *++cs) {
			int n = 0;
			switch(c) {
				static const char hex[] = "0123456789ABCDEF";
				char l[4];
			default  :
				if(isprint(c)) {
					if(!esc || !strchr(esc, c)) {
						s = StringAppendChar(t, c);
						if(!s) break;
						t = s;
						continue;
					}
					l[n++] = '\\';
					l[n++] = c;
					goto common;
				}
				l[n++] = '\\';
				l[n++] = 'x';
				l[n++] = hex[(c >> 4) & 0x0F];
				l[n++] = hex[ c       & 0x0F];
				goto common;
			case '\n':
				l[n++] = '\\';
				l[n++] = 'n';
				goto common;
			case '\t':
				l[n++] = '\\';
				l[n++] = 't';
				goto common;
			case '\\':
				l[n++] = '\\';
				l[n++] = '\\';
			common:
				s = StringAppendCharLiteral(t, l, n);
				if(!s) break;
				t = s;
				continue;
			}
			break;
		}
	}

	if(endp) {
		*endp = (char *)cs;
	}

	return t;
}

uint64_t
HashString(
	String s
) {
	size_t      n;
	char const *cs = StringToCharLiteral(s, &n);
	return memhash(cs, n, 0);
}

//------------------------------------------------------------------------------

size_t
codepointoffset(
	char   const *cs,
	size_t const  n,
	size_t        r
) {
	size_t const m = utf8len(cs, NULL, n);
	return ~m ? utf8off(cs, NULL, (r < m) ? r : m) : 0;
}

size_t
reversecodepointoffset(
	char   const *cs,
	size_t const  n,
	size_t        r
) {
	size_t const m = utf8len(cs, NULL, n);
	return ~m ? utf8off(cs, NULL, (r < m) ? (m - r) : 0) : 0;
}

