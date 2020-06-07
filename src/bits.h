#ifndef BITS_H_INCLUDED
#define BITS_H_INCLUDED
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

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

static inline int
popcount64(
	uint64_t x
) {
	// https://en.wikipedia.org/wiki/Hamming_weight
	uint64_t const m1 = UINT64_C(0x5555555555555555);
	uint64_t const m2 = UINT64_C(0x3333333333333333);
	uint64_t const m4 = UINT64_C(0x0F0F0F0F0F0F0F0F);
	x -= (x >> 1) & m1;              // put count of each 2 bits into those 2 bits
	x  = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
	x  = (x + (x >> 4)) & m4;        // put count of each 8 bits into those 8 bits
	x += x >>  8;                    // put count of each 16 bits into their lowest 8 bits
	x += x >> 16;                    // put count of each 32 bits into their lowest 8 bits
	x += x >> 32;                    // put count of each 64 bits into their lowest 8 bits
	return x & 0x7f;
}

static inline int
popcount32(
	uint32_t x
) {
	// https://en.wikipedia.org/wiki/Hamming_weight
	uint32_t const m1 = UINT32_C(0x55555555);
	uint32_t const m2 = UINT32_C(0x33333333);
	uint32_t const m4 = UINT32_C(0x0F0F0F0F);
	x -= (x >> 1) & m1;              // put count of each 2 bits into those 2 bits
	x  = (x & m2) + ((x >> 2) & m2); // put count of each 4 bits into those 4 bits
	x  = (x + (x >> 4)) & m4;        // put count of each 8 bits into those 8 bits
	x += x >>  8;                    // put count of each 16 bits into their lowest 8 bits
	x += x >> 16;                    // put count of each 32 bits into their lowest 8 bits
	return x & 0x7f;
}

static inline size_t
popcountz(
	size_t x
) {
#if SIZE_MAX > UINT32_MAX
	return popcount64(x);
#else
	return popcount32(x);
#endif
}

static inline uint64_t
bitmask64(
	uint64_t x
) {
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);
	return x;
}

static inline uint32_t
bitmask32(
	uint32_t x
) {
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x;
}

static inline size_t
bitmaskz(
	size_t x
) {
#if SIZE_MAX > UINT32_MAX
	return bitmask64(x);
#else
	return bitmask32(x);
#endif
}

static inline int
lzcount64(
	uint64_t x
) {
	int n = 64;

	if(x >= (UINT64_C(1) << 32)) {
		x >>= 32;
		n -= 32;
	}
	if(x >= (UINT64_C(1) << 16)) {
		x >>= 16;
		n -= 16;
	}
	if(x >= (UINT64_C(1) << 8)) {
		x >>= 8;
		n -= 8;
	}
	if(x >= (UINT64_C(1) << 4)) {
		x >>= 4;
		n -= 4;
	}
	x *= 4;
	x  = UINT64_C(0x4444444433332210) >> x;
	x &= 0xf;
	n -= x;

	return n;
}

static inline int
lzcount32(
	uint32_t x
) {
	int n = 32;

	if(x >= (UINT32_C(1) << 16)) {
		x >>= 16;
		n -= 16;
	}
	if(x >= (UINT32_C(1) << 8)) {
		x >>= 8;
		n -= 8;
	}
	if(x >= (UINT32_C(1) << 4)) {
		x >>= 4;
		n -= 4;
	}
	x *= 4;
	x  = UINT64_C(0x4444444433332210) >> x;
	x &= 0xf;
	n -= x;

	return n;
}

static inline size_t
lzcountz(
	size_t x
) {
#if SIZE_MAX > UINT32_MAX
	return lzcount64(x);
#else
	return lzcount32(x);
#endif
}

static inline int
tzcount64(
	uint64_t x
) {
	int n = 64;

	x &= -x;
	n -= (x > 0);

	if(x & UINT64_C(0x00000000FFFFFFFF)) n -= 32;
	if(x & UINT64_C(0x0000FFFF0000FFFF)) n -= 16;
	if(x & UINT64_C(0x00FF00FF00FF00FF)) n -=  8;
	if(x & UINT64_C(0x0F0F0F0F0F0F0F0F)) n -=  4;
	if(x & UINT64_C(0x3333333333333333)) n -=  2;
	if(x & UINT64_C(0x5555555555555555)) n -=  1;

	return n;
}

static inline int
tzcount32(
	uint32_t x
) {
	int n = 32;

	x &= -x;
	n -= (x > 0);

	if(x & UINT32_C(0x0000FFFF)) n -= 16;
	if(x & UINT32_C(0x00FF00FF)) n -=  8;
	if(x & UINT32_C(0x0F0F0F0F)) n -=  4;
	if(x & UINT32_C(0x33333333)) n -=  2;
	if(x & UINT32_C(0x55555555)) n -=  1;

	return n;
}

static inline size_t
tzcountz(
	size_t x
) {
#if SIZE_MAX > UINT32_MAX
	return tzcount64(x);
#else
	return tzcount32(x);
#endif
}

static inline int
msbit64(
	uint64_t x
) {
#	define MSBIT_SHIFT(V,N)  (((V) > (((uint64_t)1 << (1 << (N))) - 1)) << (N))

	int      n, shift;
	uint64_t v;

	n     = MSBIT_SHIFT(x,5);
	v     = x >> n;
	shift = MSBIT_SHIFT(v,4);
	v   >>= shift;
	n    |= shift;
	shift = MSBIT_SHIFT(v,3);
	v   >>= shift;
	n    |= shift;
	shift = MSBIT_SHIFT(v,2);
	v   >>= shift;
	n    |= shift;
	shift = MSBIT_SHIFT(v,1);
	v   >>= shift;
	n    |= shift;
	n    |= (v >> 1);

	return n + (x > 0);

#	undef MSBIT_SHIFT
}

static inline int
msbit32(
	uint32_t x
) {
#	define MSBIT_SHIFT(V,N)  (((V) > (((uint32_t)1 << (1 << (N))) - 1)) << (N))

	int      n, shift;
	uint32_t v;

	n     = MSBIT_SHIFT(x,4);
	v     = x >> n;
	shift = MSBIT_SHIFT(v,3);
	v   >>= shift;
	n    |= shift;
	shift = MSBIT_SHIFT(v,2);
	v   >>= shift;
	n    |= shift;
	shift = MSBIT_SHIFT(v,1);
	v   >>= shift;
	n    |= shift;
	n    |= (v >> 1);

	return n + (x > 0);

#	undef MSBIT_SHIFT
}

static inline size_t
msbitz(
	size_t x
) {
#if SIZE_MAX > UINT32_MAX
	return msbit64(x);
#else
	return msbit32(x);
#endif
}

//------------------------------------------------------------------------------

static inline bool
is_at_capacity(
	size_t n
) {
	return (n & (n - 1)) == 0;
}

static inline bool
isnot_at_capacity(
	size_t n
) {
	return (n & (n - 1)) != 0;
}

static inline size_t
capacity_of(
	size_t n
) {
	if(isnot_at_capacity(n)) {
		return bitmaskz(n) + 1;
	}

	return n;
}

//------------------------------------------------------------------------------

static inline uintptr_t
tag(
	uintptr_t ptr
) {
	return ptr | 1;
}

static inline uintptr_t
untag(
	uintptr_t ptr
) {
	return ptr & ~(uintptr_t)1;
}

static inline uintptr_t
entag(
	uintptr_t ptr
) {
	return ptr << 1;
}

static inline uintptr_t
detag(
	uintptr_t ptr
) {
	return ptr >> 1;
}

static inline uintptr_t
tag_pointer(
	void const *ptr
) {
	return tag((uintptr_t)ptr);
}

static inline void *
untag_pointer(
	uintptr_t ptr
) {
	return (void *)untag(ptr);
}

static inline bool
is_tagged(
	uintptr_t ptr
) {
	return (ptr & 1) != 0;
}

static inline bool
isnot_tagged(
	uintptr_t ptr
) {
	return (ptr & 1) == 0;
}

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef BITS_H_INCLUDED
