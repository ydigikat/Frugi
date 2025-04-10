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


#include "trace.h"


#include "frugi_params.h"
#include "dsp_core.h"
#include "dsp_math.h"


struct frugi_osc
{
  /* I/O */
  float *sample_buf;
  float *lfo_values;
  float *env_level;  

  /* Parameters */
  enum frugi_osc_wave wave;
  float octave;
  float semi;
  float cents;
  float pw;
  float level;

  enum frugi_lfo_function lfo_function;
  float lfo_depth;
  float env_depth;
  float pwm_depth;


  /* Private Data */
  uint32_t fsr;
  float phase;
  float inc;
  float pitch; 

  uint8_t id;
  
};




/* API */
void frugi_osc_init(struct frugi_osc *osc, float fsr, float *sample_buf, float *lfo_values, float *env_level, uint8_t id);
void frugi_osc_reset(struct frugi_osc *osc);
void frugi_osc_render(struct frugi_osc *osc, size_t block_size);
void frugi_osc_note_on(struct frugi_osc *osc, float pitch);
void frugi_osc_note_off(struct frugi_osc *osc);
void frugi_osc_update_params(struct frugi_osc *osc, float waveform, float octave, float semi, float cents, 
  float pw, float level, float depth, float function, float env_depth);

  void frugi_osc_update_params_internal(struct frugi_osc *osc);  



#endif /* OSC_H */
