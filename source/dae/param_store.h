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

#ifndef __PARAM_H__
#define __PARAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dae.h"

void param_set(uint16_t id, float norm_value);
void param_set_midi(uint16_t id, uint8_t value);

float param_get(uint16_t id);

/* Conversions */
#define PARAM_TO_INT(norm, min, max) (((int)((norm * ((max) - (min)) + 0.5f)) + (min)))
#define PARAM_TO_LINEAR(norm, min, max) (min + (max-min)*norm)
#define PARAM_TO_EXP(norm,min,max) (min * powf(max/min,norm))
#define PARAM_TO_POWER(norm, min, max,exp) (min+(max-min)*powf(norm,exp))


#endif /* __PARAM_H__ */