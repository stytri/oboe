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
#include "lex.h"
#include "utf8.h"

//------------------------------------------------------------------------------

char const *
lex(
	char const  *cs,
	char const **endp,
	char const **linep,
	unsigned    *linop
) {
	char const *start, *end;
	char32_t    c;

restart:
	c = utf8chr(start = cs, &cs);
restart_continue:
	for(; c && ~c && is_Space(c); c = utf8chr(start = cs, &cs))
		;
	if(is_Digit(c)) {
		int (*is__digit)(char32_t c) = is_Digit;
		end = cs;

		if(c == '0') {
			c = utf8chr(end = cs, &cs);
			if((c == 'X') || (c == 'x')) {

				is__digit = is_XDigit;
				c = utf8chr(end = cs, &cs);
			}
		}

		for(; is__digit(c); c = utf8chr(end = cs, &cs))
			;

		int is__float = (c == '.');
		if(is__float) {
			c = utf8chr(cs, &cs);
			if(!is__digit(c)) {
				*endp = end;
				goto done;
			}

			for(; is__digit(c); c = utf8chr(end = cs, &cs))
				;
		}

		if((is__digit == is_XDigit) ? (
				(c == 'P') || (c == 'p')
			) : (
				(c == 'E') || (c == 'e')
			)
		) {
			c = utf8chr(cs, &cs);
			if(is__float && ((c == '+') || (c == '-'))) {
				c = utf8chr(cs, &cs);
			}

			if(!is_Digit(c)) {
				*endp = end;
				goto done;
			}

			for(; is_Digit(c); c = utf8chr(end = cs, &cs))
				;
		}

		*endp = end;

	} else if(is_ID_Start(c)) {

		do {
			end = cs;

			c = utf8chr(cs, &cs);

		} while(is_ID_Continue(c))
			;
		*endp = end;

	} else switch(c) {
	default :
		if(!is_Operator(c)) {
			goto restart;
		}

		do {
			c = utf8chr(end = cs, &cs);
		} while(is_Operator(c))
			;
		*endp = end;
		break;

	case '\0':
		*endp = cs;
		break;

	case '\n':
		if(*cs == '\r') {
			++cs;
		}
		*linep = cs;
		++*linop;
		goto restart;
	case '\r':
		if(*cs == '\n') {
			++cs;
		}
		*linep = cs;
		++*linop;
		goto restart;
	case '\f':
	case '\v':
	case 0x85:   // NEL
	case 0x2028: // LS
	case 0x2029: // PS
		*linep = cs;
		++*linop;
		goto restart;

	case '#':
		c = utf8chr(cs, &cs);
		if((c == '(') || (c == '[') || (c == '{')) {
			char32_t const ec    = (c == '(') ? ')' : (c == '[') ? ']' : '}';
			char32_t const sc    = c;
			size_t         depth = 1;
			do {
				c = *++cs;
				if(c == sc) {
					++depth;
				} else if(c == ec) {
					--depth;
					if(depth == 0) {
						c = *++cs;
					}
				}
			} while(c && (depth > 0))
				;
			goto restart;
		}

		for(; c && ~c && !is_EOL(c); c = utf8chr(start = cs, &cs))
			;
		goto restart_continue;

	case '(': case ')':
	case '[': case ']':
	case '{': case '}':
	case ';': case ',':
		*endp = cs;
		break;

	case '"':
		for(c = *cs; c && (c != '"'); c = *++cs)
			;
		if(c) {
			++cs;
		}
		*endp = cs;
		break;
	case '\'': case '`':
		{
			char32_t const e = c;
			for(c = *cs; c && (c != e); c = *++cs) {
				if(c == '\\') {
					c = *++cs;
					if(!c) {
						break;
					}
				}
			}
		}
		if(c) {
			++cs;
		}
		*endp = cs;
		break;
	}

done:
	return start;
}

