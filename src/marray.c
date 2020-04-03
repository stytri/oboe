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
#include "xmem.h"
#include "bits.h"

//------------------------------------------------------------------------------

bool
marray_expand(
	Array  arr,
	size_t size,
	size_t count
) {
	size_t const buf_max  = XMEM_MAX / size;
	size_t       capacity = arr ? arr->capacity : buf_max;

	if((0 < count) && (count <= (buf_max - capacity))) {
		size_t new_capacity      = capacity_of(capacity + count);
		size_t mask_capacity     = bitmaskz   (capacity);
		size_t tail              = popcountz  (mask_capacity);
		size_t mask_new_capacity = bitmaskz   (new_capacity);
		size_t new_tail          = popcountz  (mask_new_capacity);
		void **slot              = arr->base;

		if(!slot || (tail < new_tail)) {
			slot = xrealloc(arr->base, (new_tail * sizeof(void *)));
			if(slot) {
				arr->base = slot;

				if(capacity == 0) {
					slot[0] = xmalloc(size);
					if(!slot[0]) {
						return false;
					}
					capacity = arr->capacity = 1;
					++tail;
				}

				for(; tail < new_tail; ++tail) {
					slot[tail] = xmalloc(capacity * size);
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
		void **slot = arr->base;
		if(slot) {
			size_t m = bitmaskz (arr->capacity);
			size_t n = popcountz(m);
			for(size_t i = 0; i < n; ++i) {
				xfree(slot[i]);
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
	size_t m = bitmaskz(index);
	size_t i = popcountz(m);
	size_t x = index & (m >> 1);
	void **s = arr->base;

	return &((char *)(s[i]))[x * size];
}

