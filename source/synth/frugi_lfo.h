/*
  ------------------------------------------------------------------------------
   Frugi
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission to use, copy, modify, and/or distribute this code for any purpose
   with or without fee is hereby granted, provided the above copyright notice and
   this permission notice appear in all copies.
  ------------------------------------------------------------------------------
*/

#ifndef __LFO_H__
#define __LFO_H__

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>


#include "trace.h"



#include "frugi_params.h"
#include "dsp_core.h"
#include "dsp_math.h"
#include "midi.h"

/* Ranges */
#define LFO_MIN_RATE (0)
#define LFO_MAX_RATE (20)

struct frugi_lfo
{
  /* I/O  */
  float *lfo_values;

  /* Parameters */
  float rate;
  enum frugi_lfo_mode mode;
  
  /* Private data */
  float phase;
  float inc;  
  float fsr;
  float hold_time;
  float sh_value;
  float prev_phase;
};


void frugi_lfo_init(struct frugi_lfo *lfo, float fsr, float *lfo_buffer_o);
void frugi_lfo_reset(struct frugi_lfo *lfo);
void frugi_lfo_render(struct frugi_lfo *lfo, size_t block_size);
void frugi_lfo_update_params(struct frugi_lfo *lfo, float rate, float mode);
void frugi_lfo_note_on(struct frugi_lfo *lfo);



#endif // __LFO_H