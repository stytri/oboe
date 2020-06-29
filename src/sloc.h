#ifndef SLOC_H_INCLUDED
#define SLOC_H_INCLUDED
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

#include "stdtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef uint64_t sloc_t;

//------------------------------------------------------------------------------

static inline sloc_t
sloc_max(
	sloc_t limit,
	sloc_t value
) {
	return (value > limit) ? limit : value;
}

static inline sloc_t
make_sloc(
	unsigned long source,
	unsigned long line,
	unsigned      offset,
	unsigned      count
) {
	return sloc_max(0x000FFFu, count)
		| (sloc_max(0x000FFFu, offset) << 12)
		| (sloc_max(0x0FFFFFu, line)   << 24)
		| (sloc_max(0x0FFFFFu, source) << 44)
	;
}

static inline unsigned long
sloc_source(
	sloc_t sloc
) {
	return (unsigned long)(sloc >> 44) & 0x0FFFFFlu;
}

static inline unsigned long
sloc_line(
	sloc_t sloc
) {
	return (unsigned long)(sloc >> 24) & 0x0FFFFFlu;
}

static inline unsigned
sloc_offset(
	sloc_t sloc
) {
	return (unsigned)(sloc >> 12) & 0x000FFFu;
}

static inline unsigned
sloc_count(
	sloc_t sloc
) {
	return (unsigned)sloc & 0x000FFFu;
}

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef SLOC_H_INCLUDED
