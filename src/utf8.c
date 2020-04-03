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
#include "utf8.h"

//------------------------------------------------------------------------------

#define UTF8LEN(C)  ((UINT64_C(0x4322000011111111) >> (((C) >> 4) * 4)) & 0xF)

//------------------------------------------------------------------------------

size_t
utf8qlen(
	char const  *cs,
	char const **endp,
	size_t       maxlen
) {
	size_t len = 0;

	for(; len < maxlen; ++len) {
		char32_t c = *cs;
		if(!c) goto end;

		++cs;
		if((c & 0x80) == 0) {
			continue;
		}

		for(c = *cs; (c & 0xC0) == 0x80; c = *++cs)
			;
	}
end:
	*endp = cs;
	return len;
}

size_t
utf8len(
	char const  *cs,
	char const **endp,
	size_t       maxlen
) {
	size_t len = 0;

	for(; len < maxlen; ++len) {
		char32_t  c = *cs;
		int const n = UTF8LEN(c);
		switch(n) {
		case 1:
			if(!c) goto end;

			++cs;
			continue;
		case 2:
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			++cs;
			continue;
		case 3:
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			++cs;
			continue;
		case 4:
			if(c > 0xF7)           goto err;
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			c = *++cs;
			if((c & 0xC0) != 0x80) goto err;
			++cs;
			continue;
		case 0:
		default:
			for(c = *++cs; (c & 0xC0) == 0x80; c = *++cs)
				;
		err:
			*endp = cs;
			return ~(size_t)0;
		}
	}
end:
	*endp = cs;
	return len;
}

char32_t
utf8chr(
	char const  *cs,
	char const **endp
) {
	char32_t  c = *cs;
	int const n = UTF8LEN(c);
	switch(n) {
		char32_t cc;
	case 1:
		*endp = cs + !!c;
		return c;
	case 2:
		cc = (c & 0x1F) << 6;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F);
		*endp = cs + 1;
		return cc;
	case 3:
		cc = (c & 0x0F) << 12;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F) << 6;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F);
		*endp = cs + 1;
		return cc;
	case 4:
		if(c > 0xF7)           goto err;
		cc = (c & 0x07) << 18;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F) << 12;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F) << 6;
		c = *++cs;
		if((c & 0xC0) != 0x80) goto err;
		cc |= (c & 0x3F);
		*endp = cs + 1;
		return cc;
	case 0:
	default:
		for(c = *++cs; (c & 0xC0) == 0x80; c = *++cs)
			;
	err:
		*endp = cs;
		return ~(char32_t)0;
	}
}

