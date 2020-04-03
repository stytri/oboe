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
#include "gc.h"
#include "array.h"

//------------------------------------------------------------------------------

struct gc_stats gc_stats = GC_STATS();

//------------------------------------------------------------------------------

// Bit  0    Mark bit
// Bit  1    Leaf flag
// Bits 2..  Link address

enum { MARK_MASK = 0x01u };
enum { LEAF_FLAG = 0x02u };
enum { TAG_BITS  = LEAF_FLAG | MARK_MASK };

static inline void *
toptr(
	uintptr_t uip
) {
	return (void *)(uip & ~(uintptr_t)TAG_BITS);
}

//------------------------------------------------------------------------------

static struct array gc_stack = ARRAY();
static uintptr_t    gc_list  = (uintptr_t)&gc_list;
static uintptr_t    gc_tag   = 0x0;

//------------------------------------------------------------------------------

//                     +---------+        +---------+
//  +---------+        |  xmem   |        |  xmem   |     +---------+
//  | gc_list | --+    | header  | --+    | header  | --> | gc_list |
//  +---------+   |    +---------+   |    +---------+     +---------+
//                +--> |         |   +--> |         |
//                               |                  |
//                     |                  |
//                     |         |        |         |
//                     +---------+        +---------+

//------------------------------------------------------------------------------

void
gc(
	void (*mark)(
		void const *p
	),
	void (*sweep)(
		void const *p
	)
) {
	gc_stats.marked = 0;
	gc_stats.swept = 0;
	gc_stats.kept  = 0;

	if(gc_list != (uintptr_t)&gc_list) {
		gc_tag ^= MARK_MASK;

		for(size_t i = gc_stack.length; i-- > 0; ) {
			void const *curr = array_at(&gc_stack, void const *, i);

			if(curr) {
				gc_mark(mark, curr);
			}
		}

		uintptr_t *prev = &gc_list;
		for(void const *next, *curr = toptr(gc_list); curr != &gc_list; curr = next) {
			uintptr_t *up = xmemheader(curr);

			next = toptr(*up);

			if((*up & MARK_MASK) == gc_tag) {
				prev = up;

				++gc_stats.kept;

			} else {
				*prev = (uintptr_t)next | (*prev & TAG_BITS);

				sweep(curr);

				++gc_stats.swept;
			}
		}
	}

	return;
}

void
gc_mark(
	void      (*mark)(
		void const *p
	),
	void const *p
) {
	uintptr_t *up = xmemheader(p);

	if((*up & MARK_MASK) != gc_tag) {
		*up ^= MARK_MASK;

		if((*up & LEAF_FLAG) == 0) {
			mark(p);
		}
	}

	++gc_stats.marked;

	return;
}

void *
gc_leaf(
	void const *p
) {
	uintptr_t *up = xmemheader(p);

	if(!*up) {
		*up = gc_list | LEAF_FLAG | gc_tag;

		gc_list = (uintptr_t)p;

		gc_stats.live++;
		gc_stats.total++;
	}

	return (void *)p;
}

void *
gc_branch(
	void const *p
) {
	uintptr_t *up = xmemheader(p);

	if(!*up) {
		*up = gc_list | gc_tag;

		gc_list = (uintptr_t)p;

		gc_stats.live++;
		gc_stats.total++;
	}

	return (void *)p;
}

void *
gc_remove(
	void const *p
) {
	uintptr_t *up = xmemheader(p);

	if(((*up & ~(uintptr_t)TAG_BITS) != 0)
		&& ((*up ^ ~(uintptr_t)TAG_BITS) != 0)
	) {
		*up = 0;

		gc_stats.live--;
		gc_stats.dead++;
	}

	return (void *)p;
}

bool
gc_is_leaf(
	void const *p
) {
	uintptr_t const *up = xmemheader(p);

	return (*up & LEAF_FLAG) != 0;
}

inline void *
gc_push(
	void const *p
) {
	return array_push_back(&gc_stack, void const *, p) ? (
		(void *)p
	) : (
		NULL
	);
}

inline void
gc_revert(
	size_t ts
) {
	if(gc_stack.length > ts) {
		gc_stack.length = ts;
	}
}

extern void *
gc_return(
	size_t      ts,
	void const *p
) {
	gc_revert(ts);

	return gc_push(p);
}


size_t
gc_topof_stack(
	void
) {
	return gc_stack.length;
}

