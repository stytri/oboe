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
#include <string.h>
#include <time.h>
#include "rand.h"

//------------------------------------------------------------------------------

static inline uint64_t
rol64(
	uint64_t u,
	int      n
) {
	return (u << n) | (u >> (64 - n));
}

//------------------------------------------------------------------------------

static uint64_t state[4];

//------------------------------------------------------------------------------

// https://en.wikipedia.org/wiki/Xorshift#Initialization

static uint64_t
splitmix64_next(
	void
) {
	uint64_t r = state[3];

	state[3] =  r + 0x9E3779B97f4A7C15;
	r        = (r ^ (r >> 30)) * 0xBF58476D1CE4E5B9;
	r        = (r ^ (r >> 27)) * 0x94D049BB133111EB;

	return r ^ (r >> 31);
}

static void
splitmix64_seed(
	unsigned seed
) {
	state[0] = state[1] = state[2] = state[3] = seed;
}

//------------------------------------------------------------------------------

// https://en.wikipedia.org/wiki/Xorshift#xoshiro256**

static uint64_t
xoshiro256ss_next(
	void
) {
	uint64_t const r = rol64(state[1] * 5, 7) * 9;
	uint64_t const t = state[1] << 17;

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;
	state[3] = rol64(state[3], 45);

	return r;
}

static void
xoshiro256ss_seed(
	unsigned seed
) {
	splitmix64_seed(seed);

	state[0] = splitmix64_next();
	state[1] = splitmix64_next();
	state[2] = splitmix64_next();
	state[3] = splitmix64_next();
}

//------------------------------------------------------------------------------

// https://burtleburtle.net/bob/rand/smallprng.html

static uint64_t
jsf64_next(
	void
) {
	uint64_t t = state[0] - rol64(state[1],  7);
	state[0]   = state[1] ^ rol64(state[2], 13);
	state[1]   = state[2] + rol64(state[3], 37);
	state[2]   = state[3] + t;
	state[3]   = state[0] + t;

	return state[3];
}

static void
jsf64_seed(
	unsigned seed
) {
	splitmix64_seed(seed);

	state[0] = UINT64_C(0xf1ea5eed);
	state[1] = splitmix64_next();
	state[2] = splitmix64_next();
	state[3] = splitmix64_next();

	for(int i = 20; i--; jsf64_next())
		;
}

//------------------------------------------------------------------------------

// http://pracrand.sourceforge.net/

static uint64_t
sfc64_next(
	void
) {
	uint64_t t = state[0] + state[1] + state[3]++;
	state[0]   = state[1] ^ (state[1] >> 11);
	state[1]   = state[2] + (state[2] <<  7);
	state[2]   = rol64(state[2], 24) + t;

	return t;
}

static void
sfc64_seed(
	unsigned seed
) {
	splitmix64_seed(seed);

	state[0] = splitmix64_next();
	state[1] = splitmix64_next();
	state[2] = splitmix64_next();
	state[3] = 1;

	for(int i = 12; i--; sfc64_next())
		;
}

//------------------------------------------------------------------------------

// http://pracrand.sourceforge.net/

#define XSM64_K UINT64_C(0xA3EC647659359ACD)

static inline void
xsm64_step_forward(
	void
) {
	uint64_t t = state[2] + state[1];
	state[2]  += state[0];
	state[3]  += t + ((state[2] < state[0]) ? 1 : 0);
}

static uint64_t
xsm64_next(
	void
) {
	uint64_t t = state[3] ^ rol64(state[3] + state[2], 16);
	t         ^= rol64(t + state[1], 40);
	t         *= XSM64_K;

	xsm64_step_forward();

	t         ^= rol64(t + state[3], 32);
	t         *= XSM64_K;
	t         ^= t >> 32;

	return t;
}

static void
xsm64_seed(
	unsigned seed
) {
	splitmix64_seed(seed);

	uint64_t s1 = splitmix64_next();
	uint64_t s2 = splitmix64_next();
	uint64_t s3 = splitmix64_next();

	state[0]  = (s1 <<  1) | 1;
	state[1]  = (s1 >> 63) | (s2 << 1);
	state[2]  = state[0];
	state[3]  = state[1] ^ ((s2 >> 63) << 63);

	xsm64_step_forward();

	state[3] += s3 << 31;
}

//------------------------------------------------------------------------------

void     (*rand_seed)(unsigned) = splitmix64_seed;
uint64_t (*rand_next)(void)     = splitmix64_next;

//------------------------------------------------------------------------------

void
initialise_rand(
	char const *generator
) {
	static const struct {
		char const *name;
		void      (*seed)(unsigned);
		uint64_t  (*next)(void);
	} table[] = {
#	define GENERATOR(Lexeme, Name) { Lexeme, Name##_seed, Name##_next }
		GENERATOR("default"     , splitmix64),
		GENERATOR("jsf"         , jsf64),
		GENERATOR("sfc"         , sfc64),
		GENERATOR("splitmix"    , splitmix64),
		GENERATOR("xoshiro256**", xoshiro256ss),
		GENERATOR("xsm"         , xsm64),
#	undef GENERATOR
	};

	size_t i = 0;
	if(generator && *generator) {
		for(i = sizeof(table) / sizeof(table[0]);
			i-- && strcmp(generator, table[i].name);
		);
	}
	rand_seed = table[i].seed;
	rand_next = table[i].next;

	unsigned s = (unsigned)time(NULL);
	rand_seed(s);
}

uint64_t
rand_in_range(
	uint64_t r,
	uint64_t range
) {
	if(range == 0 || r == range) {
		return 0;
	}

	if(r > range) {
		for(uint64_t t = -range % range;
			r < t;
			r = rand_next()
		);

		r %= range;
	}

	return r;
}

double
rand_to_float(
	uint64_t r,
	double   range
) {
	return (double)(r >> 11) * range * (1.0 / (double)((uint64_t)1 << 53));
}

