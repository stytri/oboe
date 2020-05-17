#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED
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
#include "assert.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

static_assert(sizeof(size_t) <= sizeof(uintptr_t), "sizeof(size_t) <= sizeof(uintptr_t)");

//------------------------------------------------------------------------------

typedef struct array *Array;
struct array {
	void     *base;
	size_t    length;
	size_t    capacity;
	uintptr_t map;
};
#define ARRAY(...)  (struct array){ NULL, 0, 0, (uintptr_t)NULL }

//------------------------------------------------------------------------------

static inline size_t
array_length(
	Array arr
) {
	return arr ? arr->length : 0;
}

static inline size_t
array_capacity(
	Array arr
) {
	return arr ? arr->capacity : 0;
}

static inline size_t
array_available(
	Array arr
) {
	return arr ? (arr->capacity - arr->length) : 0;
}

static inline bool
array_at_capacity(
	Array arr
) {
	return arr ? (arr->length == arr->capacity) : true;
}

//------------------------------------------------------------------------------

extern bool
array_expand(
	Array  arr,
	size_t size,
	size_t count
);

extern void
array_clear(
	Array arr
);

extern void
array_free(
	Array arr
);

//------------------------------------------------------------------------------

extern size_t
array_map_index(
	Array       arr,
	uint64_t    hash,
	size_t      index
);

extern size_t
array_get_index(
	Array       arr,
	uint64_t    hash,
	int       (*cmp)(
		Array       arr,
		size_t      index,
		void const *key,
		size_t      n
	),
	void const *key,
	size_t      n
);

extern int
array_foreach(
	Array arr,
	int (*callback)(void *, size_t, uint64_t),
	void *context
);

//------------------------------------------------------------------------------

#define array_ptr(Arr,Type,Index)  ( \
	(Type *)(((Array)(Arr))->base) + (Index) \
)

#define array_at(Arr,Type,Index)  ( \
	((Type *)(((Array)(Arr))->base))[Index] \
)

#define array_create_back(Arr,Type)  ( \
	(array_at_capacity(Arr) ? ( \
		array_expand((Arr), sizeof(Type), 1) \
	) : ( \
		true \
	)) ? ( \
		array_ptr((Arr),Type,(Arr)->length++) \
	) : ( \
		(Type *)NULL \
	) \
)

#define array_push_back(Arr,Type,Val)  ( \
	(array_at_capacity(Arr) ? ( \
		array_expand((Arr), sizeof(Type), 1) \
	) : ( \
		true \
	)) ? ( \
		array_at((Arr),Type,(Arr)->length++) = (Val), \
		true \
	) : ( \
		false \
	) \
)

#define array_append(Arr,Type,Vec,Len)  ( \
	((array_available(Arr) < (Len)) ? ( \
		array_expand((Arr), sizeof(Type), (Len)) \
	) : ( \
		true \
	)) ? ( \
		memcpy(&array_at((Arr),Type,(Arr)->length), (Vec), (sizeof(Type) * (Len))), \
		(Arr)->length += (Len), \
		true \
	) : ( \
		false \
	) \
)

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef ARRAY_H_INCLUDED
