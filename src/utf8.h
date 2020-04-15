#ifndef UTF8_H_INCLUDED
#define UTF8_H_INCLUDED
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
#include <stddef.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

size_t
utf8off(
	char const  *cs,
	char const **endp,
	size_t       index
);

extern size_t
utf8len(
	char const  *cs,
	char const **endp,
	size_t       maxlen
);

extern char32_t
utf8chr(
	char const  *cs,
	char const **endp
);

extern size_t
utf8enlen(
	char32_t cc
);

extern size_t
utf8encode(
	char    *s,
	char   **endp,
	char32_t cc
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef UTF8_H_INCLUDED
