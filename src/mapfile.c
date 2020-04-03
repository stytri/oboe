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
#include "mapfile.h"
#include <stdio.h>
#include <errno.h>

//------------------------------------------------------------------------------

#if LONG_MAX > SIZE_MAX
#	define LTLZ(L,Z)  ((L) < (long)(Z))
#	define EQLZ(L,Z)  ((L) == (long)(Z))
#else
#	define LTLZ(L,Z)  ((size_t)(L) < (Z))
#	define EQLZ(L,Z)  ((size_t)(L) == (Z))
#endif

//------------------------------------------------------------------------------

String
mapfile(
	char const *file
) {
	FILE *fp = fopen(file, "rb");
	if(fp) {
		if(0 == fseek(fp, 0, SEEK_END)) {
			long n = ftell(fp);
			if((n > 0) && LTLZ(n, STRING_MAX)) {
				String s = StringReserve(n);
				if(s) {
					rewind(fp);
					void  *p    = (void *)StringToCharLiteral(s, NULL);
					size_t size = fread(p, 1, n, fp);
					if(EQLZ(n, size)) {
						fclose(fp);
						return s;
					}
					StringDelete(s);
				}
			} else {
				errno = ERANGE;
			}
		}
		fclose(fp);
	}

	return NULL;
}

