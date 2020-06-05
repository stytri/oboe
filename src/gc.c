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
#include "gc.h"
#include "bitmac.h"
#include <string.h>

//------------------------------------------------------------------------------

struct gc {
	uintptr_t link;
	size_t    size;
	void    (*mark )(void const *ptr, void (*gc_mark)(void const *));
	void    (*sweep)(void const *ptr);
};

//------------------------------------------------------------------------------

#define GC_MIN  BIT_ROUND(sizeof(struct gc))
#define GC_MAX  (BIT_ROUND(SIZE_MAX/2) - GC_MIN)
#define GC_TAG  ((uintptr_t)0x3)

static size_t       _gc_total_size   = 0;
static size_t       _gc_sizeof_stack = 0;
static size_t       _gc_topof_stack  = 0;
static void const **_gc_stack        = NULL;
static uintptr_t    _gc_list         = ((uintptr_t)NULL & ~GC_TAG) | 0x1;

//------------------------------------------------------------------------------

static inline bool
_gc_in_limit(
	size_t count,
	size_t size
) {
	return (GC_MAX >= _gc_total_size)
		&& (((GC_MAX - _gc_total_size) / count) >= size)
	;
}

static inline size_t
_gc_rounded_size(
	size_t size
) {
	return GC_MIN + ((size + (GC_MIN - 1)) & ~(GC_MIN - 1));
}

//------------------------------------------------------------------------------

static inline struct gc *
_gc(
	void const *ptr
) {
	return (struct gc *)ptr;
}

static inline void *
_gc_wrap(
	void const *ptr
) {
	return (void *)((char *)ptr + GC_MIN);
}

static inline void *
_gc_unwrap(
	void const *ptr
) {
	return (void *)((char *)ptr - GC_MIN);
}

static inline void
_gc_push(
	void const *ptr
) {
	if((_gc_topof_stack == _gc_sizeof_stack)
		&& (_gc_sizeof_stack <= GC_MAX)
	) {
		size_t       new_sizeof_stack = _gc_sizeof_stack ? (_gc_sizeof_stack * 2) : GC_MIN;
		void const **new_stack        = (realloc)(_gc_stack, new_sizeof_stack * sizeof(_gc_stack[0]));
		if(!new_stack) {
			return;
		}
		_gc_stack        = new_stack;
		_gc_sizeof_stack = new_sizeof_stack;
	}

	_gc_stack[_gc_topof_stack++] = ptr;

	return;
}

static inline void *
_gc_link(
	void *ptr
) {
	if(_gc(ptr)->link == 0) {
		_gc(ptr)->link = _gc_list;
		_gc_list       = ((uintptr_t)ptr & ~GC_TAG) | (_gc_list & GC_TAG);
	}
	return ptr;
}

static void
_gc_mark_callback(
	void const *ptr
);

static inline void
_gc_mark(
	void const *ptr
) {
	if((_gc_list ^ _gc(ptr)->link) & GC_TAG) {
		_gc(ptr)->link ^= GC_TAG;
		_gc(ptr)->mark(_gc_wrap(ptr), _gc_mark_callback);
	}

	return;
}

static void
_gc_mark_callback(
	void const *ptr
) {
	if(!ptr) {
		return;
	}

	_gc_mark(_gc_unwrap(ptr));

	return;
}

//------------------------------------------------------------------------------

static void
_gc_no_mark(
	void const *ptr,
	void      (*gc_mark)(void const *)
) {
	(void)ptr;
	(void)gc_mark;
}

static void
_gc_no_sweep(
	void const *ptr
) {
	(void)ptr;
}

static void
_gc_default_mark(
	void const *ptr,
	void      (*gc_mark)(void const *)
) {
	(void)ptr;
	(void)gc_mark;
}

static void
_gc_default_sweep(
	void const *ptr
) {
	(free)(_gc_unwrap(ptr));
}

static inline void *
_gc_set(
	void  *ptr,
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
) {
	_gc(ptr)->link  = 0;
	_gc(ptr)->size  = size;
	_gc(ptr)->mark  = mark  ? mark  : _gc_default_mark;
	_gc(ptr)->sweep = sweep ? sweep : _gc_default_sweep;

	return _gc_wrap(ptr);
}

//------------------------------------------------------------------------------

void *
gc_malloc(
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
) {
	void *ptr = NULL;

	size += !size;
	if(_gc_in_limit(1, size)) {
		size = _gc_rounded_size(size);
		ptr  =  (malloc)(size);
		if(ptr) {
			memset(ptr, 0, GC_MIN);
			ptr = _gc_set(ptr, size, mark, sweep);
			_gc_total_size += size;
		}
	}

	return ptr;
}

void *
gc_calloc(
	size_t count,
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
) {
	void *ptr = NULL;

	size += !size;
	count += !count;
	if(_gc_in_limit(count, size)) {
		size = _gc_rounded_size(count * size);
		ptr  =  (calloc)(1, size);
		if(ptr) {
			ptr = _gc_set(ptr, size, mark, sweep);
			_gc_total_size += size;
		}
	}

	return ptr;
}

void *
gc_realloc(
	void const *ptr,
	size_t      size,
	void      (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void      (*sweep)(void const *ptr)
) {
	if(!ptr) {
		return gc_malloc(size, mark, sweep);
	}

	ptr = _gc_unwrap(ptr);

	size += !size;
	if(_gc_in_limit(1, size)
		&& (_gc(ptr)->link == 0)
	) {
		size_t oldz = _gc(ptr)->size;
		size        = _gc_rounded_size(size);
		if(oldz == size) {
			return _gc_wrap(ptr);
		}

		ptr = (realloc)((void *)ptr, size);
		if(ptr) {
			if(oldz < size) {
				_gc_total_size += (size - oldz);
			} else {
				_gc_total_size -= (oldz - size);
			}

			_gc(ptr)->size = size;
			return _gc_wrap(ptr);
		}
	}

	return NULL;
}

void
gc_free(
	void const *ptr
) {
	if(ptr) {
		ptr = _gc_unwrap(ptr);
		if(_gc(ptr)->link == 0) {
			_gc(ptr)->mark  = _gc_no_mark;
			_gc(ptr)->sweep = _gc_no_sweep;
			_gc_total_size -= _gc(ptr)->size;
			(free)((void *)ptr);
		}
	}

	return;
}

void *
gc_pmalloc(
	void  *placement,
	size_t size,
	void (*mark )(void const *ptr, void (*gc_mark)(void const *)),
	void (*sweep)(void const *ptr)
) {
	void *ptr = NULL;

	if(size >= GC_MIN) {
		if(!(ptr = placement)) {
			if(_gc_in_limit(1, size - GC_MIN)) {
				size = _gc_rounded_size(size - GC_MIN);
				ptr  =  (malloc)(size);
			}
		}

		if(ptr) {
			memset(ptr, 0, GC_MIN);
			ptr = _gc_set(ptr, size, mark, sweep);
			_gc_total_size += size;
		}
	}

	return ptr;
}

void *
gc_pfree(
	void const *ptr
) {
	if(ptr) {
		ptr = _gc_unwrap(ptr);
		if(_gc(ptr)->link == 0) {
			_gc(ptr)->mark  = _gc_no_mark;
			_gc(ptr)->sweep = _gc_no_sweep;
			_gc_total_size -= _gc(ptr)->size;
		}
	}

	return (void *)ptr;
}

size_t
_gc_sizeof(
	size_t size
) {
	return _gc_rounded_size(size);
}

size_t
gc_size(
	void const *ptr
) {
	if(!ptr) {
		return 0;
	}

	ptr = _gc_unwrap(ptr);

	return _gc(ptr)->size - GC_MIN;
}

//------------------------------------------------------------------------------

size_t
gc_total_size(
	void
) {
	return _gc_total_size;
}

size_t
gc_topof_stack(
	void
) {
	return _gc_topof_stack;
}

void
gc_revert(
	size_t top
) {
	if(top < _gc_topof_stack) {
		_gc_topof_stack = top;
	}

	return;
}

void *
gc_return(
	size_t      top,
	void const *ptr
) {
	gc_revert(top);

	return gc_push(ptr);
}

void *
gc_push(
	void const *ptr
) {
	if(!ptr) {
		return (void *)ptr;
	}

	_gc_push(_gc_link(_gc_unwrap(ptr)));

	return (void *)ptr;
}

void *
gc_link(
	void const *ptr
) {
	if(!ptr) {
		return (void *)ptr;
	}

	_gc_link(_gc_unwrap(ptr));

	return (void *)ptr;
}

void *
gc_unlink(
	void const *ptr
) {
	return NULL;
	(void)ptr;
}

void
gc_mark_and_sweep(
	void
) {
	if((_gc_list & ~GC_TAG) == (uintptr_t)NULL) {
		return;
	}

	uintptr_t const tag = (_gc_list ^= GC_TAG) & GC_TAG;

	for(size_t i = _gc_topof_stack; i-- > 0; ) {
		_gc_mark(_gc_stack[i]);
	}

	uintptr_t *prev = &_gc_list;
	for(void *ptr, *next = (void *)(_gc_list & ~GC_TAG); (ptr = next); ) {
		next = (void *)(_gc(ptr)->link & ~GC_TAG);

		if(((_gc(ptr)->link ^ tag) & GC_TAG) == 0) {
			prev = &_gc(ptr)->link;
			continue;
		}

		*prev = (uintptr_t)next | tag;

		_gc(ptr)->link = 0;
		_gc(ptr)->sweep(_gc_wrap(ptr));
	}

	return;
}

