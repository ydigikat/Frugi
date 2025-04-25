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
#ifndef OSC_H
#define OSC_H

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>

#include "params.h"
#include "dsp_core.h"
#include "dsp_math.h"


#include "trace.h"


struct osc
{
  /* Buffers */
  float *samples;  
  float *modulators;

  /* Parameters */
  enum osc_wave wave_param;
  int octave_param;
  int semi_param;
  int cents_param;
  float level_param;
  enum mod_source mod_source_param;
  float mod_depth_param;
  float pw_param;
  
  /* Private Data */
  float fsr;
  float phase;
  float inc;
  float pitch; 

  bool reset_buf;
};

/* API */
void osc_init(struct osc *osc, float fsr, float *samples, float *modulators, bool reset_buf);
void osc_reset(struct osc *osc);
void osc_render(struct osc *osc, size_t block_size);
void osc_note_on(struct osc *osc, float pitch);
void osc_note_off(struct osc *osc);
void osc_update_params(struct osc *osc, float waveform, float octave, float semi, float cents,float level, float mod_source, float mod_depth, float pw);


#endif /* OSC_H */
