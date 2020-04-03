#ifndef BITMAC_H_INCLUDED
#define BITMAC_H_INCLUDED
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

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------

#define BIT_MASK32(X)             ((X)|((X)>>32))
#define BIT_MASK16(X)   BIT_MASK32((X)|((X)>>16))
#define BIT_MASK8(X)    BIT_MASK16((X)|((X)>>8))
#define BIT_MASK4(X)    BIT_MASK8 ((X)|((X)>>4))
#define BIT_MASK2(X)    BIT_MASK4 ((X)|((X)>>2))
#define BIT_MASK1(X)    BIT_MASK2 ((X)|((X)>>1))
#define BIT_MASK(X)     BIT_MASK1 (0ull+(X))

#define BIT_COUNT32(X)             ((((X)&0xFFFFFFFF00000000ull)>>32)+((X)&0x00000000FFFFFFFFull))
#define BIT_COUNT16(X)  BIT_COUNT32((((X)&0xFFFF0000FFFF0000ull)>>16)+((X)&0x0000FFFF0000FFFFull))
#define BIT_COUNT8(X)   BIT_COUNT16((((X)&0xFF00FF00FF00FF00ull)>>8 )+((X)&0x00FF00FF00FF00FFull))
#define BIT_COUNT4(X)   BIT_COUNT8 ((((X)&0xF0F0F0F0F0F0F0F0ull)>>4 )+((X)&0x0F0F0F0F0F0F0F0Full))
#define BIT_COUNT2(X)   BIT_COUNT4 ((((X)&0xCCCCCCCCCCCCCCCCull)>>2 )+((X)&0x3333333333333333ull))
#define BIT_COUNT1(X)   BIT_COUNT2 ((((X)&0xAAAAAAAAAAAAAAAAull)>>1 )+((X)&0x5555555555555555ull))
#define BIT_COUNT(X)    BIT_COUNT1 (0ull+(X))

#define BIT_ROUND(X)    (((X)&((X)-1))?(BIT_MASK(X)+1):(X))

//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif//ndef BITMAC_H_INCLUDED
