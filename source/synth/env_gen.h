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



#include "params.h"
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

/* We have 2 envelope generators */
#define AMP_ENV_GEN (0)
#define MOD_ENV_GEN (1)

struct env_gen
{
  /* Output value */
  float *env_level;  

  /* Parameters */
  float attack_param, decay_param, sustain_param, release_param;
  enum env_mode mode_param;
  bool note_tracking_param;
  bool velocity_tracking_param;

  /* Private data */
  enum env_state state;
  float attack_scaler;
  float decay_scaler;
  float level;
  float fsr;
  size_t block_size;

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

void env_gen_init(struct env_gen *env, float fsr, size_t block_size, float *env_level);
void env_gen_reset(struct env_gen *env);
void env_gen_render(struct env_gen *env, size_t block_size);
void env_gen_note_on(struct env_gen *env_gen, uint8_t midi_note, uint8_t midi_veloc);
void env_gen_note_off(struct env_gen *env);
void env_gen_rtz(struct env_gen *env);
void env_gen_update_params(struct env_gen *env, float attack, float decay, float sustain, float release,float mode, float note_tracking, float velocity_tracking);

#endif /* __ENV_GEN_H__*/