#ifndef GC_H_INCLUDED
#define GC_H_INCLUDED
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
#include "xmem.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern void
gc(
	void (*mark)(
		void const *p
	),
	void (*sweep)(
		void const *p
	)
);

extern void
gc_mark(
	void      (*mark)(
		void const *p
	),
	void const *p
);

extern void *
gc_leaf(
	void const *p
);

extern void *
gc_branch(
	void const *p
);

extern void *
gc_remove(
	void const *p
);

extern bool
gc_is_leaf(
	void const *p
);

extern void *
gc_push(
	void const *p
);

extern void
gc_revert(
	size_t ts
);

extern void *
gc_return(
	size_t      ts,
	void const *p
);

extern size_t
gc_topof_stack(
	void
);

//------------------------------------------------------------------------------

struct gc_stats {
	size_t live;
	size_t dead;
	size_t total;
	size_t collect;
	size_t marked;
	size_t swept;
	size_t kept;
};
#define GC_STATS(...)  (struct gc_stats){ 0, 0, 0, 0, 0, 0, 0 }

extern struct gc_stats gc_stats;

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef GC_H_INCLUDED
