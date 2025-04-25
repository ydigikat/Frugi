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
#include "dsp_core.h"


/**
 * Generate white noise in the range [-1.0, 1.0].
 * Uses the standard C library rand() function.
 */
float white_noise(void)
{
  float noise = (float)rand();
  noise = 2.0f * (noise / (float)RAND_MAX) - 1.0f;

  return noise;
}

/**
 * Perform linear interpolation.
 * Given two points (x1, y1) and (x2, y2), estimate the y-value at x.
 */
float linear_interpolate(float x1, float x2, float y1, float y2, float x)
{
  float dx = (x - x1) / (x2 - x1);
  return dx * y2 + (1 - dx) * y1;
}

/**
 * Generate a pseudo-random number using the xorshift32 algorithm.
 * Updates and returns the new state.
 */
uint32_t xorshift32(uint32_t *state)
{
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}
