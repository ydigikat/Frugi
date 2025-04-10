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

#ifndef __ENV_GEN_H
#define __ENV_GEN_H

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>

#include "trace.h"



#include "frugi_params.h"
#include "dsp_core.h"
#include "dsp_math.h"

enum env_state
{
  ENV_OFF,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE,
  ENV_SHUTDOWN,
  ENV_MAX
};

#define TRANSFORM_MAX (4)


struct frugi_env_gen
{
  /* I/O */
  float *output;

  /* Parameters */
  float attack, decay, sustain, release;
  enum frugi_env_mode mode;
  bool note_tracking;
  bool velocity_tracking;

  /* Private data */
  enum env_state state;

  float attack_scaler;
  float decay_scaler;


  float level;
  float fsr;
  float block_size;

  float attack_coeff;
  float decay_coeff;
  float release_coeff;
  float attack_tco;
  float decay_tco;
  float release_tco;
  float attack_overshoot;
  float decay_overshoot;
  float release_overshoot;
  

  float inc_shutdown;
};

void frugi_env_gen_init(struct frugi_env_gen *env, float fsr, size_t block_size, float *output);
void frugi_env_gen_reset(struct frugi_env_gen *env);
void frugi_env_gen_render(struct frugi_env_gen *env, size_t block_size);
void frugi_env_gen_note_on(struct frugi_env_gen *env_gen, uint8_t midi_note, uint8_t midi_veloc);
void frugi_env_gen_note_off(struct frugi_env_gen *env);
void frugi_env_gen_rtz(struct frugi_env_gen *env);
void frugi_env_gen_update_params(struct frugi_env_gen *env, float attack, float decay, float sustain, float release,float mode, float note_tracking, float velocity_tracking);

#endif /* __ENV_GEN_H__*/