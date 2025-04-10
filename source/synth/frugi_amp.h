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
#ifndef __AMP_H__
#define __AMP_H__

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>

#include "trace.h"

#include "frugi_params.h"
#include "dsp_core.h"


struct frugi_amp
{     
  /* I/O */ 
  float *env_level;  
  float *sample_buf;
  float *lfo_values;

  /* Parameters */
  float pan;
  float volume;
  
  enum frugi_lfo_function lfo_function;
  float lfo_depth;

  /* Private Data */
  float fsr;
  float gain;
  uint8_t velocity;
};

/* API */
void frugi_amp_init(struct frugi_amp *amp, float fsr, float *sample_buf, float *env_level, float *lfo_values);
void frugi_amp_render(struct frugi_amp *amp, size_t block_size);
void frugi_amp_update_params(struct frugi_amp *amp, float volume, float pan, float mod_depth, float lfo_function);



#endif /* __AMP_H__ */