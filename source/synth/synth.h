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

#include "dae.h"
#include "params.h"
#include "voice.h"
#include "trace.h"

/*
* If you change this value then you also need to change the synth render function which has unrolled loops. 
*/
#define MAX_VOICES (8)


struct synth
{
  uint8_t midi_channel;
    
  /* Voices */
  struct voice voice[MAX_VOICES];  
  float poly_attenuation;

  /* Buffers */
  float *voice_buffer_block;
  float *voice_modulators_block;

  /* Patch and params */
  float params[SYNTH_PARAM_MAX];
  uint8_t cc_to_param_map[128];

  /* For portamento/glissando */
  float last_note_freq;  

};

/* API */
void synth_init(struct synth *synth, float sample_rate, size_t block_size, uint8_t *midi_channel);
void synth_render(struct synth *synth, float *left, float *right, size_t block_size);
void synth_midi_message(struct synth *synth, uint8_t byte0, uint8_t byte1, uint8_t byte2);
void synth_update_params(struct synth *synth);

#endif /* __SYNTH_H__ */