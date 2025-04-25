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
#ifndef __VOICE_H__
#define __VOICE_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <string.h>

#include "FreeRTOS.h"

#include "dae.h"
#include "params.h"
#include "amp.h"
#include "osc.h"
#include "env_gen.h"
#include "lfo.h"
#include "filter.h"

#include "trace.h"



struct voice
{
  uint8_t id; /* For debugging */

  /* Control */
  float *params;
  float fsr;
  size_t block_size;

  /* Voice note management */
  int8_t age;
  bool note_on, note_pending;

  /* MIDI values */
  uint8_t current_note, pending_note;
  uint8_t current_velocity, pending_velocity;

  /* Derived values */
  float current_pitch, pending_pitch;
  float current_vel_factor, pending_vel_factor;

  /* Samples */
  __attribute__((aligned(4))) float *samples;

  /* Modulation values */
  __attribute__((aligned(4))) float *modulators;
  
  
  /* Signal chain */
  struct amp amp;
  struct env_gen amp_env;
  struct env_gen mod_env;
  struct osc osc1;
  struct osc osc2;
  struct lfo lfo;
  struct filter filter;
};

/* API */
void voice_init(struct voice *voice, float *params, float *samples, float *modulators, float fsr, size_t block_size);
void voice_reset(struct voice *voice);
void voice_render(struct voice *voice);
void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity);
void voice_note_off(struct voice *voice, uint8_t midi_note);
void voice_update_params(struct voice *voice);

#endif /* __VOICE_H__ */