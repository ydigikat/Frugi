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

#include "frugi_params.h"
#include "midi.h"
#include "params.h"

#define MIDI_CC_UNASSIGNED (255)
#define PATCH_BANK_MAX (8)

struct cc_mapping
{
  uint8_t cc;
  uint8_t param;
};

/* This is the mapping of CC numbers to Frugi parameter IDs */
static const struct cc_mapping frugi_mappings[FRUGI_PARAM_COUNT + 1] =
    {
        {20, FRUGI_OSC1_OCTAVE},
        {21, FRUGI_OSC1_SEMI},
        {22, FRUGI_OSC1_CENTS},
        {23, FRUGI_OSC1_WAVE},
        {24, FRUGI_OSC1_LFO_FUNCTION},
        {25, FRUGI_OSC1_LFO_DEPTH},
        {26, FRUGI_OSC1_PULSE_WIDTH},
        {27, FRUGI_OSC1_PWM_DEPTH},
        {28, FRUGI_OSC1_LEVEL},
        {29, FRUGI_OSC1_ENV_DEPTH},

        {30, FRUGI_OSC2_OCTAVE},
        {31, FRUGI_OSC2_SEMI},
        {32, FRUGI_OSC2_CENTS},
        {33, FRUGI_OSC2_WAVE},
        {34, FRUGI_OSC2_LFO_FUNCTION},
        {35, FRUGI_OSC2_LFO_DEPTH},
        {36, FRUGI_OSC2_PULSE_WIDTH},
        {37, FRUGI_OSC2_PWM_DEPTH},
        {38, FRUGI_OSC2_LEVEL},
        {39, FRUGI_OSC2_ENV_DEPTH},

        {40, FRUGI_NOISE_LEVEL},
        {41, FRUGI_NOISE_TYPE},

        {42, FRUGI_FILTER_MODE},
        {43, FRUGI_FILTER_ENV_AMOUNT},
        {44, FRUGI_FILTER_LFO_FUNCTION},
        {45, FRUGI_FILTER_LFO_RANGE},
        {46, FRUGI_FILTER_LFO_DEPTH},
        {47, FRUGI_FILTER_SATURATION},

        {48, FRUGI_MOD_ENV_ATTACK},
        {49, FRUGI_MOD_ENV_DECAY},
        {50, FRUGI_MOD_ENV_SUSTAIN},
        {51, FRUGI_MOD_ENV_RELEASE},
        {52, FRUGI_MOD_ENV_MODE},

        {53, FRUGI_AMP_LFO_FUNCTION},
        {54, FRUGI_AMP_LFO_DEPTH},
        {55, FRUGI_AMP_ENV_ATTACK},
        {56, FRUGI_AMP_ENV_DECAY},
        {57, FRUGI_AMP_ENV_SUSTAIN},
        {58, FRUGI_AMP_ENV_RELEASE},

        {60, FRUGI_LFO_RATE},
        {61, FRUGI_LFO_MODE},

        {62, FRUGI_FILTER_NOTE_TRACK},

        {84, FRUGI_AMP_ENV_VEL_SENS},
        {85, FRUGI_AMP_ENV_NOTE_TRACK},
        {86, FRUGI_MOD_ENV_VEL_SENS},
        {87, FRUGI_MOD_ENV_NOTE_TRACK},
        {88, FRUGI_FILTER_NOTE_TRACK},

        {MIDI_CC_FREQUENCYCUTOFF, FRUGI_FILTER_CUTOFF},
        {MIDI_CC_RESONANCE, FRUGI_FILTER_RESONANCE},
        {MIDI_CC_VOLUME, FRUGI_AMP_VOLUME},
        {MIDI_CC_PAN, FRUGI_AMP_PAN},
        {MIDI_CC_PORTAMENTO, FRUGI_PORTAMENTO},
        {MIDI_CC_PORTAMENTOTIME, FRUGI_PORTAMENTO_TIME},
        {MIDI_CC_PORTAMENTOAMOUNT, FRUGI_PORTAMENTO_AMOUNT}};

/* Populates the CC->param map array with the mappings defined in the const structure array above */
static void populate_cc_array(uint8_t map_array[])
{
  /* Start by setting all to unassigned */
  for (int i = 0; i < 128; i++)
  {
    map_array[i] = MIDI_CC_UNASSIGNED;
  }

  /* Now populate with the mappings */
  for (int i = 0; i < FRUGI_PARAM_COUNT; i++)
  {
    map_array[frugi_mappings[i].cc] = frugi_mappings[i].param;
  }
}

struct patch_parameter
{
  uint8_t param_id;
  uint8_t value;
};

/* Enum to MIDI value */
#define E2M(value, max) ((uint8_t)(((value) * 127) / (max)))

/* IMPORTANT: These must be in the exact same order as the enum of the
parameter ids.  The array is indexed directly */
static const struct patch_parameter base_patch[FRUGI_PARAM_COUNT + 1] =
    {
        {FRUGI_OSC1_OCTAVE, 64},
        {FRUGI_OSC1_SEMI, 64},
        {FRUGI_OSC1_CENTS, 64},
        {FRUGI_OSC1_WAVE, E2M(OSC_SAW, OSC_WAVE_COUNT)},
        {FRUGI_OSC1_LFO_FUNCTION, E2M(LFO_TRIANGLE, LFO_FUNCTION_COUNT)},
        {FRUGI_OSC1_LFO_DEPTH, 0},
        {FRUGI_OSC1_PULSE_WIDTH, 64},
        {FRUGI_OSC1_PWM_DEPTH, 0},
        {FRUGI_OSC1_LEVEL, 127},
        {FRUGI_OSC1_ENV_DEPTH, 0},

        {FRUGI_OSC2_OCTAVE, 64},
        {FRUGI_OSC2_SEMI, 64},
        {FRUGI_OSC2_CENTS, 64},
        {FRUGI_OSC2_WAVE, E2M(OSC_SAW, OSC_WAVE_COUNT)},
        {FRUGI_OSC2_LFO_FUNCTION, E2M(LFO_TRIANGLE, LFO_FUNCTION_COUNT)},
        {FRUGI_OSC2_LFO_DEPTH, 0},
        {FRUGI_OSC2_PULSE_WIDTH, 64},
        {FRUGI_OSC2_PWM_DEPTH, 0},
        {FRUGI_OSC2_LEVEL, 127},
        {FRUGI_OSC2_ENV_DEPTH, 0},

        {FRUGI_NOISE_LEVEL, 0},
        {FRUGI_NOISE_TYPE, E2M(NOISE_TYPE_WHITE, NOISE_TYPE_COUNT)},

        {FRUGI_FILTER_CUTOFF, 86},
        {FRUGI_FILTER_RESONANCE, 0},
        {FRUGI_FILTER_MODE, E2M(FILTER_LPF2, FILTER_MODE_COUNT)},
        {FRUGI_FILTER_ENV_AMOUNT, 0},
        {FRUGI_FILTER_LFO_FUNCTION, E2M(LFO_TRIANGLE, LFO_FUNCTION_COUNT)},
        {FRUGI_FILTER_LFO_RANGE, E2M(LFO_RANGE_NORMAL, LFO_RANGE_COUNT)},
        {FRUGI_FILTER_LFO_DEPTH, 0},
        {FRUGI_FILTER_SATURATION, 0},
        {FRUGI_FILTER_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_COUNT)},

        {FRUGI_MOD_ENV_ATTACK, 0},
        {FRUGI_MOD_ENV_DECAY, 0},
        {FRUGI_MOD_ENV_SUSTAIN, 127},
        {FRUGI_MOD_ENV_RELEASE, 0},
        {FRUGI_MOD_ENV_MODE, E2M(ENV_NORMAL, ENV_MODE_COUNT)},
        {FRUGI_MOD_ENV_VEL_SENS, E2M(SWITCH_OFF, SWITCH_COUNT)},
        {FRUGI_MOD_ENV_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_COUNT)},

        {FRUGI_AMP_VOLUME, 127},
        {FRUGI_AMP_PAN, 64},
        {FRUGI_AMP_LFO_FUNCTION, E2M(LFO_TRIANGLE, LFO_FUNCTION_COUNT)},
        {FRUGI_AMP_LFO_DEPTH, 0},
        {FRUGI_AMP_ENV_ATTACK, 0},
        {FRUGI_AMP_ENV_DECAY, 0},
        {FRUGI_AMP_ENV_SUSTAIN, 127},
        {FRUGI_AMP_ENV_RELEASE, 0},
        {FRUGI_AMP_ENV_VEL_SENS, E2M(SWITCH_OFF, SWITCH_COUNT)},
        {FRUGI_AMP_ENV_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_COUNT)},

        {FRUGI_LFO_RATE, 0},
        {FRUGI_LFO_MODE, E2M(LFO_TRIGGER, LFO_MODE_COUNT)},

        {FRUGI_PORTAMENTO, E2M(SWITCH_OFF, SWITCH_COUNT)},
        {FRUGI_PORTAMENTO_TIME, 0},
        {FRUGI_PORTAMENTO_AMOUNT, 0},
        {FRUGI_HOLD, E2M(SWITCH_OFF, SWITCH_COUNT)},
        {FRUGI_MOD_WHEEL, 0},
        {FRUGI_CHORUS_MIX,0}
};

/* Patch bank patches, these are differential - stored as variations from the base patch
   The parameters within do not have to be in any particular order as they are applied by ID */

static const struct patch_parameter patch1[] =
    {
        {FRUGI_OSC1_WAVE, E2M(OSC_SAW, OSC_WAVE_COUNT)}, /* Change to square */
        {FRUGI_OSC2_LEVEL,0}, /* Mute OSC2 */
        

        {FRUGI_FILTER_MODE, E2M(FILTER_LPF2, FILTER_MODE_COUNT)}, /* Change to HPF */
        {FRUGI_FILTER_CUTOFF, 127},                                /* Lower the cutoff */
        {FRUGI_FILTER_RESONANCE, 0},                             /* Increase resonance */

        {FRUGI_AMP_ENV_ATTACK, 0},  /* Add a short amp attack */
        {FRUGI_AMP_ENV_DECAY, 0},   /* Add a short amp decay */
        {FRUGI_AMP_ENV_SUSTAIN, 127}, /* Lower the sustain */
        {FRUGI_AMP_ENV_RELEASE, 16}, /* Add a short amp release */

        {FRUGI_AMP_ENV_VEL_SENS, E2M(SWITCH_ON, SWITCH_COUNT)},  /* Enable velocity sensitivity */
        {FRUGI_AMP_ENV_NOTE_TRACK, E2M(SWITCH_ON, SWITCH_COUNT)}, /* Disable key tracking */
        {FRUGI_FILTER_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_COUNT)},  /* Enable key tracking */
        {FRUGI_CHORUS_MIX, 64}, /* Set the chorus mix to 50% */

        {255, 255}}; /* Marks the end of the array */

static const struct patch_parameter patch2[] =
    {};

static const struct patch_parameter patch3[] =
    {};

static const struct patch_parameter patch4[] =
    {};

static const struct patch_parameter patch5[] =
    {};

static const struct patch_parameter patch6[] =
    {};

static const struct patch_parameter patch7[] =
    {};

static const struct patch_parameter patch8[] =
    {};

static const struct patch_parameter *const patch_bank[PATCH_BANK_MAX] =
    {
        patch1,
        patch2,
        patch3,
        patch4,
        patch5,
        patch6,
        patch7,
        patch8};

/* Loads the factory patch and the associated CC->param map, for Frugi there is only a single param map
shared by all patches*/
void load_factory_patch(uint8_t patch_number, uint8_t *cc_param_map)
{
  /* Start with the base patch, this contains default values for all parameters */
  for (int i = 0; i < FRUGI_PARAM_COUNT; i++)
  {
    param_set_midi(i, base_patch[i].value);
  }

  /* Now apply the patch, this will override the base patch values */
  const struct patch_parameter *patch = patch_bank[patch_number];
  while (patch->param_id != 255)
  {
    param_set_midi(patch->param_id, patch->value);
    patch++;
  }

  populate_cc_array(cc_param_map);
}
