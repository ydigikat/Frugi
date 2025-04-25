/*
  ------------------------------------------------------------------------------
   DAE
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

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
  ------------------------------------------------------------------------------
*/
#ifndef __DSP_MATH_H__
#define __DSP_MATH_H__

#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef FAST_FUNCTIONS
  #define FAST_FUNCTIONS 0
#endif

#if FAST_FUNCTIONS
  #define MATH_POW(x, y) fast_pow(x, y)
  #define MATH_EXP(x) fast_exp(x)
  #define MATH_ABS(x) fast_fabs(x)
  #define MATH_TAN(x) fast_tan(x)
  #define MATH_LOG2(x) fast_log2(x)
  #define MATH_LOG10(x) fast_log10(x)
#else
  #define MATH_POW(x, y) powf(x, y)
  #define MATH_EXP(x) expf(x)
  #define MATH_ABS(x) fabsf(x)
  #define MATH_TAN(x) tanf(x)
  #define MATH_LOG2(x) log2f(x)
  #define MATH_LOG10(x) log10f(x)
#endif

#define CONVEX_LIMIT (0.00398107f)
#define UNI_TO_BI(value) (2.0f * value - 1.0f)

#define FAST_FABS(x) ({ union { float f; uint32_t i; } u = {x}; u.i &= 0x7FFFFFFF; u.f; })
#define FAST_LOG2(x) ({ union { float f; uint32_t i; } u = {x}; \
  int exponent = ((u.i >> 23) & 0xFF) - 127; \
  float mantissa = (u.i & 0x7FFFFF) / (float)(1 << 23) + 1.0f; \
  float log2m = -0.344845f + (2.024665f * mantissa) - (0.674877f * mantissa * mantissa); \
  exponent + log2m; })

#define FAST_LOG10(x) (FAST_LOG2(x) * 0.30103f)

float concave_inverted_transform(float value);
float frequency_to_attenuation(float freq);
float attenuation_to_frequency(float atten);
float fast_tan(float x);
float fast_pow(float x, float y);
float fast_exp(float x);

#endif /* __DSP_MATH_H__ */