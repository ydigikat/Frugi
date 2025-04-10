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
#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <stdint.h>
#include "trace.h"


#include "params.h"

enum frugi_param_id
{
  FRUGI_OSC1_OCTAVE,
  FRUGI_OSC1_SEMI,
  FRUGI_OSC1_CENTS,
  FRUGI_OSC1_WAVE,
  FRUGI_OSC1_LFO_FUNCTION,
  FRUGI_OSC1_LFO_DEPTH,
  FRUGI_OSC1_PULSE_WIDTH,
  FRUGI_OSC1_PWM_DEPTH,
  FRUGI_OSC1_LEVEL,
  FRUGI_OSC1_ENV_DEPTH,

  FRUGI_OSC2_OCTAVE,
  FRUGI_OSC2_SEMI,
  FRUGI_OSC2_CENTS,
  FRUGI_OSC2_WAVE,
  FRUGI_OSC2_LFO_FUNCTION,
  FRUGI_OSC2_LFO_DEPTH,
  FRUGI_OSC2_PULSE_WIDTH,
  FRUGI_OSC2_PWM_DEPTH,
  FRUGI_OSC2_LEVEL,
  FRUGI_OSC2_ENV_DEPTH,

  FRUGI_NOISE_LEVEL,
  FRUGI_NOISE_TYPE,

  FRUGI_FILTER_CUTOFF,
  FRUGI_FILTER_RESONANCE,
  FRUGI_FILTER_MODE,
  FRUGI_FILTER_ENV_AMOUNT,
  FRUGI_FILTER_LFO_FUNCTION,
  FRUGI_FILTER_LFO_RANGE,
  FRUGI_FILTER_LFO_DEPTH,
  FRUGI_FILTER_SATURATION,
  FRUGI_FILTER_NOTE_TRACK,

  FRUGI_MOD_ENV_ATTACK,
  FRUGI_MOD_ENV_DECAY,
  FRUGI_MOD_ENV_SUSTAIN,
  FRUGI_MOD_ENV_RELEASE,
  FRUGI_MOD_ENV_MODE,
  FRUGI_MOD_ENV_VEL_SENS,
  FRUGI_MOD_ENV_NOTE_TRACK,

  FRUGI_AMP_VOLUME,
  FRUGI_AMP_PAN,
  FRUGI_AMP_LFO_FUNCTION,
  FRUGI_AMP_LFO_DEPTH,
  
  FRUGI_AMP_ENV_ATTACK,
  FRUGI_AMP_ENV_DECAY,
  FRUGI_AMP_ENV_SUSTAIN,
  FRUGI_AMP_ENV_RELEASE,  
  FRUGI_AMP_ENV_VEL_SENS,
  FRUGI_AMP_ENV_NOTE_TRACK,

  FRUGI_LFO_RATE,
  FRUGI_LFO_MODE,

  FRUGI_PORTAMENTO,
  FRUGI_PORTAMENTO_TIME,
  FRUGI_PORTAMENTO_AMOUNT,
  FRUGI_HOLD,
  FRUGI_MOD_WHEEL,
  FRUGI_CHORUS_MIX,

  FRUGI_PARAM_COUNT = FRUGI_CHORUS_MIX
};

enum frugi_switch
{
  SWITCH_OFF,
  SWITCH_ON,
  SWITCH_COUNT = SWITCH_ON
};

enum frugi_osc_wave
{
  OSC_TRIANGLE,
  OSC_SAW,
  OSC_PULSE,
  OSC_WAVE_COUNT = OSC_PULSE
};

enum frugi_lfo_function
{
  LFO_TRIANGLE,
  LFO_SAW,
  LFO_REV_SAW,
  LFO_SQUARE,
  LFO_SAMPLE_HOLD,
  LFO_FUNCTION_COUNT = LFO_SAMPLE_HOLD
};

enum frugi_filter_mode
{
  FILTER_LPF2,
  FILTER_BPF2,
  FILTER_HPF2,
  FILTER_LPF4,
  FILTER_BPF4,
  FILTER_HPF4,
  FILTER_MODE_COUNT = FILTER_HPF4
};

enum frugi_lfo_mode
{
  LFO_TRIGGER,
  LFO_FREE,
  LFO_MODE_COUNT = LFO_FREE
};

enum frugi_lfo_range
{
  LFO_RANGE_NORMAL,
  LFO_RANGE_EXTREME,
  LFO_RANGE_COUNT = LFO_RANGE_EXTREME
};

enum frugi_voice_mode
{
  VOICE_POLY,
  VOICE_MONO_NORMAL,
  VOICE_MONO_LEGATO,
  VOICE_MONO_FULL,
  VOICE_MONO_SUSTAIN,
  VOICE_MODE_COUNT = VOICE_MONO_SUSTAIN
};

enum frugi_bend_range
{
  BEND_OCTAVE,
  BEND_FIFTH,
  BEND_THIRD,
  BEND_COUNT = BEND_THIRD
};

enum frugi_mod_wheel_dest
{
  MW_PITCH,
  MW_CUTOFF,
  MW_CUTOFF_AND_PITCH,
  MW_DEST_COUNT = MW_CUTOFF_AND_PITCH
};

enum frugi_env_mode
{
  ENV_NORMAL,
  ENV_BIASED,
  ENV_INVERTED,
  ENV_BIASED_INVERTED,
  ENV_MODE_COUNT = ENV_BIASED_INVERTED
};

enum frugi_noise_type
{
  NOISE_TYPE_WHITE,
  NOISE_TYPE_PINK,
  NOISE_TYPE_COUNT = NOISE_TYPE_PINK
};

void load_factory_patch(uint8_t patch_number, uint8_t *cc_param_map);

#endif /* __PARAMETERS_H__ */