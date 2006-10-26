/* Copyright (C) 2002 Jean-Marc Valin 
   File: math_approx.c
   Various math approximation functions for Speex

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "math_approx.h"
#include "misc.h"

spx_int16_t spx_ilog2(spx_uint32_t x)
{
   int r=0;
   if (x>=(spx_int32_t)65536)
   {
      x >>= 16;
      r += 16;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 8;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 4;
   }
   if (x>=4)
   {
      x >>= 2;
      r += 2;
   }
   if (x>=2)
   {
      r += 1;
   }
   return r;
}

spx_int16_t spx_ilog4(spx_uint32_t x)
{
   int r=0;
   if (x>=(spx_int32_t)65536)
   {
      x >>= 16;
      r += 8;
   }
   if (x>=256)
   {
      x >>= 8;
      r += 4;
   }
   if (x>=16)
   {
      x >>= 4;
      r += 2;
   }
   if (x>=4)
   {
      r += 1;
   }
   return r;
}

#ifdef FIXED_POINT

/* sqrt(x) ~= 0.22178 + 1.29227*x - 0.77070*x^2 + 0.25723*x^3 (for .25 < x < 1) */
#define C0 3634
#define C1 21173
#define C2 -12627
#define C3 4215

spx_word16_t spx_sqrt(spx_word32_t x)
{
   int k=0;
   spx_word32_t rt;

   if (x<=0)
      return 0;
#if 1
   if (x>=16777216)
   {
      x>>=10;
      k+=5;
   }
   if (x>=1048576)
   {
      x>>=6;
      k+=3;
   }
   if (x>=262144)
   {
      x>>=4;
      k+=2;
   }
   if (x>=32768)
   {
      x>>=2;
      k+=1;
   }
   if (x>=16384)
   {
      x>>=2;
      k+=1;
   }
#else
   while (x>=16384)
   {
      x>>=2;
      k++;
      }
#endif
   while (x<4096)
   {
      x<<=2;
      k--;
   }
   rt = ADD16(C0, MULT16_16_Q14(x, ADD16(C1, MULT16_16_Q14(x, ADD16(C2, MULT16_16_Q14(x, (C3)))))));
   if (rt > 16383)
      rt = 16383;
   if (k>0)
      rt <<= k;
   else
      rt >>= -k;
   rt >>=7;
   return rt;
}

/* log(x) ~= -2.18151 + 4.20592*x - 2.88938*x^2 + 0.86535*x^3 (for .5 < x < 1) */


#define A1 16469
#define A2 2242
#define A3 1486

spx_word16_t spx_acos(spx_word16_t x)
{
   int s=0;
   spx_word16_t ret;
   spx_word16_t sq;
   if (x<0)
   {
      s=1;
      x = NEG16(x);
   }
   x = SUB16(16384,x);
   
   x = x >> 1;
   sq = MULT16_16_Q13(x, ADD16(A1, MULT16_16_Q13(x, ADD16(A2, MULT16_16_Q13(x, (A3))))));
   ret = spx_sqrt(SHL32(EXTEND32(sq),13));
   
   /*ret = spx_sqrt(67108864*(-1.6129e-04 + 2.0104e+00*f + 2.7373e-01*f*f + 1.8136e-01*f*f*f));*/
   if (s)
      ret = SUB16(25736,ret);
   return ret;
}


#define K1 8192
#define K2 -4096
#define K3 340
#define K4 -10

spx_word16_t spx_cos(spx_word16_t x)
{
   spx_word16_t x2;

   if (x<12868)
   {
      x2 = MULT16_16_P13(x,x);
      return ADD32(K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   } else {
      x = SUB16(25736,x);
      x2 = MULT16_16_P13(x,x);
      return SUB32(-K1, MULT16_16_P13(x2, ADD32(K2, MULT16_16_P13(x2, ADD32(K3, MULT16_16_P13(K4, x2))))));
   }
}

#include <stdio.h>
/*
 K0 = 1
 K1 = log(2)
 K2 = 3-4*log(2)
 K3 = 3*log(2) - 2
*/
#define D0 16384
#define D1 11356
#define D2 3726
#define D3 1301
/* Input in Q11 format, output in Q16 */
static spx_word32_t spx_exp2(spx_word16_t x)
{
   int integer;
   spx_word16_t frac;
   integer = SHR16(x,11);
   if (integer>14)
      return 0x7fffffff;
   else if (integer < -15)
      return 0;
   frac = SHL16(x-SHL16(integer,11),3);
   frac = ADD16(D0, MULT16_16_Q14(frac, ADD16(D1, MULT16_16_Q14(frac, ADD16(D2 , MULT16_16_Q14(D3,frac))))));
   if (integer+2>0)
      return SHL32(EXTEND32(frac), integer+2);
   else
      return SHR32(EXTEND32(frac), -integer-2);
}

spx_word32_t spx_exp(spx_word16_t x)
{
   if (x>21290)
      return 0x7fffffff;
   else if (x<-21290)
      return 0;
   else
      return spx_exp2(1+MULT16_16_P14(23637,x));
}


#else

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#define C1 0.9999932946f
#define C2 -0.4999124376f
#define C3 0.0414877472f
#define C4 -0.0012712095f


#define SPX_PI_2 1.5707963268
spx_word16_t spx_cos(spx_word16_t x)
{
   if (x<SPX_PI_2)
   {
      x *= x;
      return C1 + x*(C2+x*(C3+C4*x));
   } else {
      x = M_PI-x;
      x *= x;
      return NEG16(C1 + x*(C2+x*(C3+C4*x)));
   }
}

#endif
