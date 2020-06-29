#ifndef OPTGET_H_INCLUDED
#define OPTGET_H_INCLUDED
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

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

struct optget
{
	int  const  x;
	/* optget() return value:
	**          > 0 : option code
	**            0 : non-option argument,
	**                also used to indicate help-only messages
	**
	** optget() may also return:
	**           -1 : invalid option
	*/
	char const *s;
	/* option specification, and, help
	**
	**        Options are placed at the front of the string,
	**        they always start with -', and, may contain '-'
	**        and alpha-numeric characters. Multiple options
	**        in the same string options are separated by ','.
	**
	**        Following the options are one, or more parameter
	**        place-holders; their only use is to enumerate
	**        required parameters, they are separated by space.
	**        Parameters apply to all options in the same string.
	**
	**        Text in a negative-coded entry is aligned as per
	**        > 0 option text, but is otherwise non-functional.
	**
	**        Text in a 0-coded entry is non-functional.
	*/
	char const *t;
	/* Help text describing the option.
	*/
};

extern int
optget(
	size_t                 optc,
	struct optget const    optv[optc],
	char const  **restrict argp,
	char const   *restrict args,
	int                    argn,
	int          *restrict params
);

extern void
optuse(
	size_t                 optc,
	struct optget const    optv[optc],
	FILE         *restrict outf
);

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef OPTGET_H_INCLUDED
