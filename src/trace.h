#ifndef TRACE_H_INCLUDED
#define TRACE_H_INCLUDED
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

#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

extern void
trace(
	size_t      indent,
	Ast         ast,
	char const *comment
);

#define TRACE(Indent,Ast)  \
	do{ \
		if(trace_enabled) { \
			trace((Indent), (Ast), NULL); \
		} \
	} while(0)
#define TRACE_COMMENT(Indent,Comment)  \
	do{ \
		if(trace_enabled) { \
			trace((Indent), NULL, (Comment)); \
		} \
	} while(0)
#define TRACE_WITH_COMMENT(Indent,Ast,Comment)  \
	do{ \
		if(trace_enabled) { \
			trace((Indent), (Ast), (Comment)); \
		} \
	} while(0)
#define TRACE_IF(Cond,Indent,Ast)  \
	do{ \
		if(trace_enabled && (Cond)) { \
			TRACE((Indent), (Ast)); \
		} \
	} while(0)
#define TRACE_IF_COMMENT(Cond,Indent,Comment)  \
	do{ \
		if(trace_enabled && (Cond)) { \
			TRACE_COMMENT((Indent), (Comment)); \
		} \
	} while(0)
#define TRACE_IF_WITH_COMMENT(Cond,Indent,Ast,Comment)  \
	do{ \
		if(trace_enabled && (Cond)) { \
			TRACE_WITH_COMMENT((Indent), (Ast), (Comment)); \
		} \
	} while(0)

//------------------------------------------------------------------------------

extern bool   trace_enabled;
extern size_t trace_global_indent;

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef TRACE_H_INCLUDED
