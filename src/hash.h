#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED
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
#include "nobreak.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

static inline uint64_t
memhash(
	void const *buf,
	size_t      len,
	uint64_t    seed
) {
	uint64_t const  M = UINT64_C(2891462833508853929);
	uint64_t        h = (seed * M) + (len * M);
	uint64_t const *w = (uint64_t const *)buf;
	uint64_t        x1, x2, x3, x4;

	for(uint64_t const *const end = w + ((len & ~31u) >> 3); w != end; ) {
		x1  = *w++;
		x2  = *w++;
		x3  = *w++;
		x4  = *w++;
		h  *= M;
		x1 *= M;
		h  += x1;
		h  *= M;
		x2 *= M;
		h  += x2;
		h  *= M;
		x3 *= M;
		h  += x3;
		h  *= M;
		x4 *= M;
		h  += x4;
	}

	if(len & 16u) {
		x1  = *w++;
		x2  = *w++;
		h  *= M;
		x1 *= M;
		h  += x1;
		h  *= M;
		x2 *= M;
		h  += x2;
	}

	if(len & 8u) {
		x1  = *w++;
		h  *= M;
		x1 *= M;
		h  += x1;
	}

	x1 = 0;
	switch(len & 7u) {
	case 7: x1 |= (uint64_t)*(((uint8_t const *)w) + 6) << 48; nobreak;
	case 6: x1 |= (uint64_t)*(((uint8_t const *)w) + 5) << 40; nobreak;
	case 5: x1 |= (uint64_t)*(((uint8_t const *)w) + 4) << 32; nobreak;
	case 4: x1 |= (uint64_t)*(((uint8_t const *)w) + 3) << 24; nobreak;
	case 3: x1 |= (uint64_t)*(((uint8_t const *)w) + 2) << 16; nobreak;
	case 2: x1 |= (uint64_t)*(((uint8_t const *)w) + 1) <<  8; nobreak;
	case 1: x1 |= (uint64_t)*(((uint8_t const *)w)    );
		h  *= M;
		x1 *= M;
		h  += x1;
		nobreak;
	default:
		break;
	}

	return (h >> 32) - h;
}

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef HASH_H_INCLUDED
