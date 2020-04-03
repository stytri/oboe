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
#include "xmem.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

//------------------------------------------------------------------------------

static size_t xmem_total_size = 0;

//------------------------------------------------------------------------------

static inline bool
xmem_in_limit(
	size_t count,
	size_t size
) {
	return (
		   (  XMEM_MAX >= xmem_total_size)
		&& (((XMEM_MAX  - xmem_total_size) / count) >= size)
	);
}

static inline size_t
xmem_rounded_size(
	size_t size
) {
	return (size + (XMEM_MIN - 1)) & ~(XMEM_MIN - 1);
}

static inline struct xmem *
xmem(
	void const *p
) {
	return (struct xmem *)((char *)p - XMEM_MIN);
}

//------------------------------------------------------------------------------

size_t
xmemtotal(
	void
) {
	return xmem_total_size;
}

inline size_t
xmemsize(
	void const *p
) {
	return p ? (
		xmem(p)->size
	) : (
		0
	);
}

void *
xmalloc(
	size_t size
) {
	size += !size;
	if(xmem_in_limit(1, size)) {

		size = xmem_rounded_size(size);
		void *p = malloc(XMEM_MIN + size);
		if(p) {

			xmem_total_size += (XMEM_MIN + size);
			((struct xmem *)p)->size = size;
			((struct xmem *)p)->header = 0;
			return (char *)p + XMEM_MIN;
		}
	}

	return NULL;
}

void *
xcalloc(
	size_t count,
	size_t size
) {
	size += !size;
	count += !count;
	if(xmem_in_limit(count, size)) {

		size *= count;
		size  = xmem_rounded_size(size);
		count = size / XMEM_MIN;
		void *p = calloc(count + 1, XMEM_MIN);
		if(p) {

			xmem_total_size += (XMEM_MIN + size);
			((struct xmem *)p)->size = size;
			return (char *)p + XMEM_MIN;
		}
	}

	return NULL;
}

void *
xrealloc(
	void  *p,
	size_t size
) {
	if(!p) {
		return xmalloc(size);
	}
	size += !size;
	if(xmem_in_limit(1, size)
		&& (xmem(p)->header == 0)
	) {

		size_t oldz = xmemsize(p);
		size = xmem_rounded_size(size);
		if(oldz == size) {
			return p;
		}

		p = realloc(xmem(p), XMEM_MIN + size);
		if(p) {

			if(oldz < size) {
				xmem_total_size += (size - oldz);
			} else {
				xmem_total_size -= (oldz - size);
			}
			((struct xmem *)p)->size = size;
			return (char *)p + XMEM_MIN;
		}
	}

	return NULL;
}

void
xfree(
	void const *p
) {
	if(p
		&& (xmem(p)->header == 0)
	) {
		xmem_total_size -= (XMEM_MIN + xmemsize(p));
		free(xmem(p));
	}
}

uintptr_t *
xmemheader(
	void const *p
) {
	return p ? (
		&xmem(p)->header
	) : (
		NULL
	);
}

