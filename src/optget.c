/*
MIT License

Copyright (c) 2015 Tristan Styles

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
#include "optget.h"
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

//------------------------------------------------------------------------------

static bool
optcmp(
	int                  onec,
	char const *restrict opts,
	char const *restrict args,
	int        *restrict params
) {
	bool         found = false;
	size_t       i     = 0;
	size_t const n     = onec ? 1 : strlen(args);

	for(; isspace(opts[i]); ++i)
		;

	while(isgraph(opts[i])) {

		if('-' == opts[i]) {
			i += !!onec;

			size_t const x = i++;

			for(; isalnum(opts[i]) || ('-' == opts[i]); ++i)
				;
			if(!found) {
				if(((i - x) == n)
					&& (0 == memcmp(opts + x, args, n))
				) {
					found = true;
				}
			}

			for(; isspace(opts[i]); ++i)
				;
		}

		if(',' != opts[i]) {
			break;
		}

		for(++i; isspace(opts[i]); ++i)
			;
	}

	for(; isgraph(opts[i]); ++*params) {

		for(++i; isgraph(opts[i]); ++i)
			;
		for(; isspace(opts[i]); ++i)
			;
	}

	return found;
}

static int
do_optget(
	size_t                 optc,
	struct optget const    optv[optc],
	char const  **restrict argp,
	char const   *restrict args,
	int                    argn,
	int          *restrict params
) {
	int r    = -1;
	int onec = (argp && *argp);

	*params = 0;

	if(onec) {
		args = *argp;

	} else if(argp) {
		int const c = *(args + 1);

		if(isalnum(c)) {
			*argp = ++args;
			onec = 1;
		}
	}

	for(size_t i = 0; optc > i; ++i) {
		struct optget const *const optp = &optv[i];

		if(optp->x > 0) {
			if(!optp->s) {
				break;
			}

			if(optcmp(onec, optp->s, args, params)) {

				if(!*params
					|| (argn >= *params)
				) {
					r = optp->x;
				}

				if(onec) {
					++*argp;

					if(!**argp) {
						*argp = NULL;
					}
				}

				break;
			}

			*params = 0;
		}
	}

	return r;
}

int
optget(
	size_t                 optc,
	struct optget const    optv[optc],
	char const  **restrict argp,
	char const   *restrict args,
	int                    argn,
	int          *restrict params
) {
	int r = -1;

	if((optc > 0)
		&& optv
		&& args
		&& (argn >= 0)
		&& params
	) {
		r = 0;

		if('-' == *args) {
			if(*(args + 1)) {
				if(*(args + 2)
					|| ('-' != *(args + 1))
				) {
					r = do_optget(optc, optv, argp, args, argn, params);
				}
			}
		}
	}

	return r;
}

//------------------------------------------------------------------------------

static int
optputs(
	struct optget const    *optp,
	size_t                  stop,
	FILE          *restrict outf
) {
	size_t n = 0;

	if(optp->s) {
		if(optp->x != 0) {
			fputs("  ", outf);
			n += 2;
		}

		fputs(optp->s, outf);
		n += strlen(optp->s);
	}

	if(optp->t) {
		if(!optp->s) {
			stop += 2;
		}

		for(stop += 2; stop > n; --stop) {
			fputc(' ', outf);
		}

		fputs("  ", outf);
		fputs(optp->t, outf);
	}

	fputc('\n', outf);

	return 0;
}

void
optuse(
	size_t                 optc,
	struct optget const    optv[optc],
	FILE         *restrict outf
) {
	if((optc > 0)
		&& optv
		&& outf
	) {
		size_t stop = 0;

		for(size_t i = 0; optc > i; ++i) {
			if(optv[i].x) {
				char const *const s = optv[i].s;
				size_t      const n = s ? strlen(s) : 0;

				if(n > stop) {
					stop = n;
				}
			}
		}

		for(size_t i = 0; optc > i; ++i) {
			if(EOF == optputs(&optv[i], stop, outf)) {
				break;
			}
		}
	}

	return;
}
