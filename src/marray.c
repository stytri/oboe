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
#include "marray.h"
#include "bits.h"
#include "gc.h"

//------------------------------------------------------------------------------

bool
marray_expand(
	Array  arr,
	size_t size,
	size_t count
) {
	size_t const buf_max  = (SIZE_MAX/2) / size;
	size_t       capacity = arr ? arr->capacity : buf_max;

	if((0 < count) && (count <= (buf_max - capacity))) {
		size_t new_capacity      = capacity_of   (capacity + count);
		size_t tail              = (size_t)msbitz(capacity);
		size_t new_tail          = (size_t)msbitz(new_capacity);
		void **slot              = arr->base;

		if(!slot || (tail < new_tail)) {
			slot = realloc(arr->base, (new_tail * sizeof(void *)));
			if(slot) {
				arr->base = slot;

				if(capacity == 0) {
					slot[0] = malloc(size);
					if(!slot[0]) {
						return false;
					}
					capacity = arr->capacity = 1;
					++tail;
				}

				for(; tail < new_tail; ++tail) {
					slot[tail] = malloc(capacity * size);
					if(!slot[tail]) {
						return false;
					}

					arr->capacity += capacity;
					capacity = arr->capacity;
				}

				return true;
			}
		}
	}

	return false;
}

void
marray_free(
	Array  arr
) {
	if(arr) {
		void **const slot = arr->base;
		if(slot) {
			size_t n = (size_t)msbitz(arr->capacity);
			for(size_t i = 0; i < n; ++i) {
				free(slot[i]);
			}
		}

		array_free(arr);
	}
}

//------------------------------------------------------------------------------

void *
marray_element_pointer(
	Array  arr,
	size_t size,
	size_t index
) {
	size_t const i = (size_t)msbitz(index);
	size_t const m = (~SIZE_C(0) >> 1) >> ((CHAR_BIT * sizeof(index)) - i);
	size_t const x = index & m;
	void **const s = arr->base;

	return &((char *)(s[i]))[x * size];
}

