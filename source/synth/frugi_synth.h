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
#ifndef __SYNTH_H__
#define __SYNTH_H__

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>

#include "frugi_params.h"
#include "frugi_voice.h"

#include "trace.h"



/* We limit to fewer voices when no optimisation for debugging */
#ifdef DEBUG
#define MAX_VOICES (3)
#else
#define MAX_VOICES (8)
#endif

struct frugi_synth
{
  uint8_t midi_channel;
    
  /* Voices */
  struct frugi_voice voice[MAX_VOICES];  
  float poly_attenuation;

  /* Patch and params */
  float params[FRUGI_PARAM_COUNT];
  uint8_t cc_to_param_map[128];

  /* For portamento/glissando */
  float last_note_freq;  

};

/* API */
void frugi_synth_init(struct frugi_synth *synth, float sample_rate, size_t block_size, uint8_t *midi_channel);
void frugi_synth_render(struct frugi_synth *synth, float *left, float *right, size_t block_size);
void frugi_synth_midi_message(struct frugi_synth *synth, uint8_t byte0, uint8_t byte1, uint8_t byte2);
void frugi_synth_update_params(struct frugi_synth *synth);

#endif /* __SYNTH_H__ */