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

#include "params.h"
#include "dsp_core.h"
#include "dsp_math.h"
#include "midi.h"

/* Ranges */
#define LFO_MIN_RATE (0)
#define LFO_MAX_RATE (20)

struct lfo
{
  /* Buffers */
  float *modulators;

  /* Parameters */
  float rate_param;
  enum lfo_trigger_mode trigger_mode_param;
  
  /* Private data */
  float phase;
  float inc;  
  float fsr;
  float hold_time;
  float sh_value;
  float prev_phase;
};


void lfo_init(struct lfo *lfo, float fsr, float *modulators);
void lfo_reset(struct lfo *lfo);
void lfo_render(struct lfo *lfo, size_t block_size);
void lfo_update_params(struct lfo *lfo, float rate, float trigger_mode);
void lfo_note_on(struct lfo *lfo);


#endif // __LFO_H