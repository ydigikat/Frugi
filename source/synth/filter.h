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

#ifndef __FILTER_H__
#define __FILTER_H__

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>
#include <math.h>


#include "trace.h"

#include "params.h"
#include "dsp_core.h"
#include "dsp_math.h"

enum filter_tap_col
{
  A_TAP = 0,
  B_TAP,
  C_TAP,
  D_TAP,
  E_TAP
};

struct sub_filter
{
  float alpha;
  float beta;
  float a0;
  float z1;
};

struct filter
{

  /* Buffers */
  float *samples;
  float *modulators;

  /* User parameters */
  enum filter_type filter_type_param;
  float cutoff_param;
  float resonance_param;
  float saturation_param;

  enum mod_source mod_source_param;
  float mod_depth_param;
  bool note_tracking_param;
  
  /* Private data */
  float fsr;
  float cutoff;
  float resonance;
  float bass_comp;
  float alpha0;
  uint8_t note;

  /* Filter cascade */
  struct sub_filter f1, f2, f3, f4;
};

void filter_init(struct filter *filter, float fsr, float *samples, float *modulators);
void filter_reset(struct filter *filter);
void filter_note_on(struct filter *filter, uint8_t note);
void filter_render(struct filter *filter, size_t block_size);
void filter_update_params(struct filter *filter, float mode, float cutoff, float resonance,
  float saturation, float mod_depth, float mod_source, float note_tracking);

#endif /* __FILTER_H__ */