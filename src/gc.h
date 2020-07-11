#ifndef GC_H_INCLUDED
#define GC_H_INCLUDED
/*
MIT License

Copyright (c) 2020 Tristan Styles

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
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

typedef void (*gc_mark_t )(void const *ptr, void (*mark)(void const *));
typedef void (*gc_sweep_t)(void const *ptr);

//------------------------------------------------------------------------------

#undef  malloc
#define malloc(Size)  gc_malloc((Size), NULL, NULL)
extern void *
gc_malloc(
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
);

#undef  calloc
#define calloc(Count,Size)  gc_calloc((Count), (Size), NULL, NULL)
extern void *
gc_calloc(
	size_t count,
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
);

#undef  realloc
#define realloc(Ptr,Size)  gc_realloc((Ptr), (Size), NULL, NULL)
extern void *
gc_realloc(
	void const *ptr,
	size_t      size,
	void      (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void      (*sweep)(void const *ptr)
);

#undef  free
#define free(Ptr)  gc_free((Ptr))
extern void
gc_free(
	void const *ptr
);

#undef  pmalloc
#define pmalloc(Placement,Size)  gc_pmalloc((Placement), (Size), NULL, NULL)
extern void *
gc_pmalloc(
	void  *placement,
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
);

#undef  pfree
#define pfree(Ptr)  gc_pfree((Ptr))
extern void*
gc_pfree(
	void const *ptr
);

#define gc_sizeof(Type)  _gc_sizeof(sizeof(Type))
extern size_t
_gc_sizeof(
	size_t size
);

extern size_t
gc_size(
	void const *ptr
);

//------------------------------------------------------------------------------

extern size_t
gc_total_size(
	void
);

extern size_t
gc_topof_stack(
	void
);

extern void
gc_revert(
	size_t top
);

extern void *
gc_return(
	size_t      top,
	void const *ptr
);

extern void *
gc_push(
	void const *ptr
);

extern void *
gc_link(
	void const *ptr
);

extern void *
gc_unlink(
	void const *ptr
);

extern void
gc_mark_and_sweep(
	void
);

//------------------------------------------------------------------------------

struct gc_stats {
	size_t size;
	size_t size_max;
	size_t size_allocated;
	size_t size_deallocated;
	size_t stack_depth;
	size_t object_live;
	size_t object_born;
	size_t object_died;
};

extern struct gc_stats const *
gc_stats(
	void
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef GC_H_INCLUDED
