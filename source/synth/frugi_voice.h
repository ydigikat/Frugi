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

#include "frugi_params.h"
#include "frugi_osc.h"
#include "frugi_amp.h"
#include "frugi_env_gen.h"
#include "frugi_lfo.h"
#include "frugi_noise.h"
#include "frugi_filter.h"

#ifdef RTT_ENABLED
#include "trace.h"
#endif

struct frugi_voice
{
  uint8_t id; /* For debugging */

  /* Control */
  float *params;
  float fsr;
  size_t block_size;

  /* Voice note management */
  uint8_t age;
  bool note_on, note_pending;

  /* MIDI values */
  uint8_t current_note, pending_note;
  uint8_t current_velocity, pending_velocity;

  /* Derived values */
  float current_pitch, pending_pitch;
  float current_vel_factor, pending_vel_factor;

  /* Buffers */
  float *sample_buf;
  float amp_env_level;
  float mod_env_level;
  float lfo_values[5];

  /* Signal chain */
  alignas(4) struct frugi_osc osc1;
  alignas(4) struct frugi_osc osc2;
  alignas(4) struct frugi_amp amp;
  alignas(4) struct frugi_env_gen amp_env;
  alignas(4) struct frugi_env_gen mod_env;
  alignas(4) struct frugi_lfo lfo;
  alignas(4) struct frugi_noise noise;
  alignas(4) struct frugi_filter filter;
};

/* API */
void frugi_voice_init(struct frugi_voice *voice, float *params, float fsr, size_t block_size);
void frugi_voice_reset(struct frugi_voice *voice);
void frugi_voice_render(struct frugi_voice *voice);
void frugi_voice_note_on(struct frugi_voice *voice, uint8_t midi_note, uint8_t midi_velocity);
void frugi_voice_note_off(struct frugi_voice *voice, uint8_t midi_note);
void frugi_voice_update_params(struct frugi_voice *voice);

#endif /* __VOICE_H__ */