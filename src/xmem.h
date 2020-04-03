#ifndef XMEM_H_INCLUDED
#define XMEM_H_INCLUDED
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
#include "bitmac.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

struct xmem {
	size_t    size;
	uintptr_t header;
};

enum {
	XMEM_MIN = (sizeof(struct xmem) & (sizeof(struct xmem) - 1)) ? (
		BIT_MASK(sizeof(struct xmem)) + 1
	) : (
		sizeof(struct xmem)
	)
};

enum {
	XMEM_MAX = (BIT_ROUND(SIZE_MAX / 2) - (CHAR_BIT * sizeof(size_t)))
};

#define XMEM_STRUCT(Type,Name) \
struct xmem__##Name##__struct { \
	struct xmem xmem; \
	Type        data; \
}

#define XMEM__(Scope,Type,Name,...) \
Scope XMEM_STRUCT(Type, Name) xmem__##Name##__actual = { \
	sizeof(struct xmem__##Name##__struct), \
	~(uintptr_t)0, \
	__VA_ARGS__ \
}; \
Scope Type *const Name = &xmem__##Name##__actual.data;

#define XMEM(Type,Name,...)         XMEM__(      ,Type,Name,__VA_ARGS__)
#define XMEM_STATIC(Type,Name,...)  XMEM__(static,Type,Name,__VA_ARGS__)

//------------------------------------------------------------------------------

extern void *
xmalloc(
	size_t size
);

extern void *
xcalloc(
	size_t count,
	size_t size
);

extern void *
xrealloc(
	void  *p,
	size_t size
);

extern void
xfree(
	void const *p
);

extern size_t
xmemsize(
	void const *p
);

extern size_t
xmemtotal(
	void
);

extern uintptr_t *
xmemheader(
	void const *p
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef XMEM_H_INCLUDED
