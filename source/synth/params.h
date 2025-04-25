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

#include "param_store.h"
#include "trace.h"

/* Gain staging (not really params but convenient to place centrally here )*/
#define OSC_GAIN_SCALER (0.5f)      /* 2 oscillators so 50% each */
#define SYNTH_GAIN_SCALER (0.75f)   /* Allow some overall headroom */

/* Crude level perception scalers, this really should use an RMS calculation but this will suffice */
#define SAW_GAIN_SCALER (2.5f)      /* Saw perceived loudness compensation */  
#define TRI_GAIN_SCALER (1.0f)      /* Triangle perceived loudness compensation*/
#define PULSE_GAIN_SCALER (0.5f)    /* Pulse perceived loudness compensation */


enum param_id
{
  OSC1_WAVE,
  OSC1_OCTAVE,
  OSC1_SEMI,
  OSC1_CENTS,  
  OSC1_LEVEL,
  OSC1_MOD_SOURCE,
  OSC1_MOD_DEPTH,
  OSC1_PW,

  OSC2_WAVE,
  OSC2_OCTAVE,
  OSC2_SEMI,
  OSC2_CENTS,  
  OSC2_LEVEL,
  OSC2_MOD_SOURCE,
  OSC2_MOD_DEPTH,
  OSC2_PW,

  FILTER_CUTOFF,
  FILTER_RESONANCE,
  FILTER_TYPE,  
  FILTER_MOD_SOURCE,  
  FILTER_MOD_DEPTH,
  FILTER_SATURATION,
  FILTER_NOTE_TRACK,

  AMP_VOLUME,
  AMP_MOD_SOURCE,
  AMP_MOD_DEPTH,

  AMP_ENV_ATTACK,
  AMP_ENV_DECAY,
  AMP_ENV_SUSTAIN,
  AMP_ENV_RELEASE,  
  AMP_ENV_VEL_SENS,
  AMP_ENV_NOTE_TRACK,

  MOD_ENV_ATTACK,
  MOD_ENV_DECAY,
  MOD_ENV_SUSTAIN,
  MOD_ENV_RELEASE,  
  MOD_ENV_VEL_SENS,
  MOD_ENV_NOTE_TRACK,
  MOD_ENV_MODE,

  LFO_RATE,
  LFO_TRIGGER_MODE,

  SYNTH_PARAM_MAX
};

enum mod_source
{
  MOD_AMP_ENV_LEVEL=0,  /* Keep this first, the modules (other than amp) start their mod sources from 1 */
  MOD_LFO_TRIANGLE,
  MOD_LFO_SAW,
  MOD_LFO_REV_SAW,
  MOD_LFO_SQUARE,
  MOD_LFO_SANDH,
  MOD_ENV_LEVEL,
  MOD_MAX_SOURCE
};

enum on_off_switch
{
  SWITCH_OFF,
  SWITCH_ON,
  SWITCH_MAX
};

enum osc_wave
{
  OSC_TRIANGLE,
  OSC_SAW,
  OSC_PULSE,
  OSC_WAVE_MAX
};

enum env_mode
{
  ENV_NORMAL,
  ENV_BIASED,
  ENV_INVERTED,
  ENV_BIASED_INVERTED,
  ENV_MODE_MAX
};

enum lfo_trigger_mode
{
  LFO_NOTE,
  LFO_FREE,
  LFO_MODE_MAX
};

enum filter_type
{
  FILTER_LPF2,
  FILTER_BPF2,
  FILTER_HPF2,
  FILTER_LPF4,
  FILTER_BPF4,
  FILTER_HPF4,
  FILTER_TYPE_MAX
};

void load_factory_patch(uint8_t patch_number, uint8_t *cc_param_map);

#endif /* __PARAMETERS_H__ */