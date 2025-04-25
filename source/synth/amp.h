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

#include "params.h"
#include "dsp_core.h"


struct amp
{     
  /* Buffers */  
  float *samples;  
  float *modulators;

  /* Parameters */
  float volume_param;
  enum mod_source mod_source_param;
  float mod_depth_param;

  /* Private Data */
  float fsr;
  float gain;
  uint8_t velocity;
};

/* API */
void amp_init(struct amp *amp, float fsr, float *samples, float *modulators);
void amp_render(struct amp *amp, size_t block_size);
void amp_update_params(struct amp *amp, float volume, float mod_source, float mod_depth);

#endif /* __AMP_H__ */