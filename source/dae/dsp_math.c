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
#include "dsp_math.h"

/**
 * \brief Fast approximation of the natural logarithm.
 * \param x Input value.
 * \return Natural logarithm of x.
 * \details This function approximates the natural logarithm of x using a
 * polynomial approximation. The approximation is accurate to within 0.005.
 * \note This function is only accurate for values between 0.5 and 1.5.
 */
float concave_inverted_transform(float value)
{
  if (value <= CONVEX_LIMIT)
  {
    return 1.0f;
  }

  return -(5.0f / 12.0f) * MATH_LOG10(value);
}


/**
 * \brief Fast approximation of tan(x).
 * \param x Input value.
 * \return Tangent of x.
 * \details This function approximates the tangent of x using a polynomial
 * approximation. The approximation is accurate to within 0.005.
 */
inline float fast_tan(float x)
{
  float x2 = x * x;
  float a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
  float b = 135135.0f - x2 * (62370.0f - x2 * (3150.0f - x2 * 28.0f));
  return a / b;
}

/**
 * \brief Fast approximation of pow(x).
 * \param x Base value.
 * \param y Exponent value.
 * \return x raised to the power of y.
 */
inline float fast_pow(float x, float y)
{
  int i = (int)y;
  float frac = y - i;
  float result = 1.0f;

  while (i--)
    result *= x;

  return result * (1.0f + frac * (x - 1.0f)); // Linear interpolation
}

/**
 * \brief Fast approximation of exp(x).
 * \param x Input value.
 * \return Exponential of x.
 * \details This function approximates the exponential of x using a polynomial
 * approximation. The approximation is accurate to within 0.005.
 */
inline float fast_exp(float x)
{
  x = 1.0f + x / 1024.0f;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  x *= x;
  return x;
}

/**
 * frequency_to_attenuation
 * \brief Convert a frequency value to an attenuation value.
 * \param freq Frequency value.
 * \return Attenuation value.
 */
float frequency_to_attenuation(float freq)
{
  return MATH_POW(10.0f, -freq / 20.0f);
}


