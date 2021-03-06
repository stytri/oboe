#ifndef MARRAY_H_INCLUDED
#define MARRAY_H_INCLUDED
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

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern bool
marray_expand(
	Array  arr,
	size_t size,
	size_t count
);

extern void
marray_free(
	Array arr
);

//------------------------------------------------------------------------------

#define marray_clear(Arr)                       array_clear(Arr)
#define marray_length(Arr)                      array_length(Arr)
#define marray_capacity(Arr)                    array_capacity(Arr)
#define marray_available(Arr)                   array_available((Arr)
#define marray_at_capacity(Arr)                 array_at_capacity(Arr)
#define marray_map_index(Arr,Hash,Index)        array_map_index((Arr),(Hash),(Index))
#define marray_get_index(Arr,Hash,Cmp,Key,Len)  array_get_index((Arr),(Hash),(Cmp),(Key),(Len))
#define marray_foreach(Arr,Callback,Context)    array_foreach((Arr),(Callback),(Context))

//------------------------------------------------------------------------------

extern void *
marray_element_pointer(
	Array  arr,
	size_t size,
	size_t index
);

//------------------------------------------------------------------------------

#define marray_ptr(Arr,Type,Index)  ( \
	(Type *)marray_element_pointer((Array)(Arr), sizeof(Type), (Index)) \
)

#define marray_at(Arr,Type,Index)  ( \
	*marray_ptr(Arr, Type, Index) \
)

#define marray_alloc_back(Arr,sizeof_Type)  ( \
	(marray_at_capacity(Arr) ? ( \
		marray_expand((Arr), sizeof_Type, 1) \
	) : ( \
		true \
	)) ? ( \
		marray_element_pointer((Arr), sizeof_Type, (Arr)->length++) \
	) : ( \
		NULL \
	) \
)

#define marray_create_back(Arr,Type)  ( \
	(Type *)marray_alloc_back(Arr, sizeof(Type)) \
)

#define marray_push_back(Arr,Type,Val)  ( \
	(marray_at_capacity(Arr) ? ( \
		marray_expand((Arr), sizeof(Type), 1) \
	) : ( \
		true \
	)) ? ( \
		marray_at((Arr),Type,(Arr)->length) = (Val), \
		(Arr)->length++, \
		true \
	) : ( \
		false \
	) \
)

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef MARRAY_H_INCLUDED
