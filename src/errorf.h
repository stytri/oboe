#ifndef ERRORF_H_INCLUDED
#define ERRORF_H_INCLUDED
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

#include <stdio.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

#ifdef __GNUC__
#	if __USE_MINGW_ANSI_STDIO
#		define ERRORF_ATTRIBUTES(F,V)	__attribute__((format(__MINGW_PRINTF_FORMAT,F,V)))
#	else
#		define ERRORF_ATTRIBUTES(F,V)	__attribute__((format(printf,F,V)))
#	endif
#else
#	define ERRORF_ATTRIBUTES(F,V)
#endif

extern void
errorf(
	char const *restrict fmt,
	...
)
	ERRORF_ATTRIBUTES(1,2);

#undef ERRORF_ATTRIBUTES

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef ERRORF_H_INCLUDED
