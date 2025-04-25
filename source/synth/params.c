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

#include "params.h"
#include "midi.h"
#include "params.h"

#define PATCH_BANK_MAX (8)

struct cc_mapping
{
  uint8_t cc;
  uint8_t param;
};

/* This is the mapping of CC numbers to Frugi parameter IDs */
static const struct cc_mapping mappings[SYNTH_PARAM_MAX] =
    {
        {20, OSC1_WAVE},
        {21, OSC1_OCTAVE},
        {22, OSC1_SEMI},
        {23, OSC1_CENTS},
        {24, OSC1_LEVEL},
        {25, OSC1_MOD_SOURCE},
        {26, OSC1_MOD_DEPTH},

        {30, OSC2_WAVE},
        {31, OSC2_OCTAVE},
        {32, OSC2_SEMI},
        {33, OSC2_CENTS},
        {34, OSC2_LEVEL},
        {35, OSC2_MOD_SOURCE},
        {36, OSC2_MOD_DEPTH},

        {42, FILTER_TYPE},        
        {44, FILTER_MOD_SOURCE},
        {46, FILTER_MOD_DEPTH},
        {47, FILTER_SATURATION},
        {MIDI_CC_FREQUENCYCUTOFF, FILTER_CUTOFF},
        {MIDI_CC_RESONANCE, FILTER_RESONANCE},

        {MIDI_CC_VOLUME, AMP_VOLUME},
        {53, AMP_MOD_SOURCE},
        {54, AMP_MOD_DEPTH},
        {55, AMP_ENV_ATTACK},
        {56, AMP_ENV_DECAY},        
        {57, AMP_ENV_SUSTAIN},
        {58, AMP_ENV_RELEASE},
        {60, LFO_RATE},
        {61, LFO_TRIGGER_MODE},
        {84, AMP_ENV_VEL_SENS},
        {85, AMP_ENV_NOTE_TRACK}};

/* Populates the CC->param map array with the mappings defined in the const structure array above */
static void populate_cc_array(uint8_t map_array[])
{
  /* Start by setting all to unassigned */
  for (int i = 0; i < 128; i++)
  {
    map_array[i] = MIDI_CC_UNSUPPORTED;
  }

  /* Now populate with the mappings */
  for (int i = 0; i < SYNTH_PARAM_MAX-1; i++)
  {
    map_array[mappings[i].cc] = mappings[i].param;
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
static const struct patch_parameter base_patch[SYNTH_PARAM_MAX] =
    {
        {OSC1_WAVE, E2M(OSC_SAW, OSC_WAVE_MAX-1)},
        {OSC1_OCTAVE, 64},
        {OSC1_SEMI, 64},
        {OSC1_CENTS, 64},        
        {OSC1_LEVEL, 64},      
        {OSC1_MOD_SOURCE,E2M(MOD_LFO_TRIANGLE, MOD_MAX_SOURCE-1)},
        {OSC1_MOD_DEPTH,0},
        {OSC1_PW,64},

        {OSC2_WAVE, E2M(OSC_SAW, OSC_WAVE_MAX-1)},
        {OSC2_OCTAVE, 64},
        {OSC2_SEMI, 64},
        {OSC2_CENTS, 64},        
        {OSC2_LEVEL, 64},      
        {OSC2_MOD_SOURCE,E2M(MOD_LFO_TRIANGLE, MOD_MAX_SOURCE-1)},
        {OSC2_MOD_DEPTH,0},
        {OSC2_PW,64},

        {FILTER_CUTOFF, 64},
        {FILTER_RESONANCE, 64},
        {FILTER_TYPE, E2M(FILTER_LPF2, FILTER_TYPE_MAX-1)},        
        {FILTER_MOD_SOURCE, E2M(MOD_LFO_TRIANGLE, MOD_MAX_SOURCE-1)},        
        {FILTER_MOD_DEPTH, 0},
        {FILTER_SATURATION, 0},
        {FILTER_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_MAX-1)},

        {AMP_VOLUME, 127},
        {AMP_MOD_SOURCE,0},
        {AMP_MOD_DEPTH,0},

        {AMP_ENV_ATTACK, 0},
        {AMP_ENV_DECAY, 0},
        {AMP_ENV_SUSTAIN, 127},
        {AMP_ENV_RELEASE, 0},
        {AMP_ENV_VEL_SENS, E2M(SWITCH_OFF, SWITCH_MAX-1)},
        {AMP_ENV_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_MAX-1)},

        {MOD_ENV_ATTACK,0},
        {MOD_ENV_DECAY,0},
        {MOD_ENV_SUSTAIN,127},
        {MOD_ENV_RELEASE,0},  
        {MOD_ENV_VEL_SENS,E2M(SWITCH_OFF, SWITCH_MAX-1)},
        {MOD_ENV_NOTE_TRACK,E2M(SWITCH_OFF, SWITCH_MAX-1)},
        {MOD_ENV_MODE, E2M(ENV_NORMAL, ENV_MODE_MAX-1)},

        {LFO_RATE,0},
        {LFO_TRIGGER_MODE,E2M(LFO_NOTE, LFO_MODE_MAX-1)}};
        
/* Patch bank patches, these are differential - stored as variations from the base patch
   The parameters within do not have to be in any particular order as they are applied by ID */

static const struct patch_parameter patch1[] =
    { 
        {OSC2_CENTS,30},
        

        /* Short attack, decay with a mid sustain and shortish release*/        
        {AMP_ENV_ATTACK, 5},    
        {AMP_ENV_DECAY, 10},     
        {AMP_ENV_SUSTAIN, 64}, 
        {AMP_ENV_RELEASE, 16},  

        {FILTER_TYPE, E2M(FILTER_LPF2,FILTER_TYPE_MAX-1)},
        {FILTER_RESONANCE,10},
        {FILTER_SATURATION, 64},
        

        /* Amp velocity but no key tracking */
        {AMP_ENV_VEL_SENS, E2M(SWITCH_ON, SWITCH_MAX-1)}, 
        {AMP_ENV_NOTE_TRACK, E2M(SWITCH_OFF, SWITCH_MAX-1)},

        /* Marks the end of the differential patch */
        {MIDI_CC_UNSUPPORTED, MIDI_CC_UNSUPPORTED}};        

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
  for (uint16_t i = 0; i < SYNTH_PARAM_MAX-1; i++)
  {
    param_set_midi(i, base_patch[i].value);
  }

  /* Now apply the patch, this will override the base patch values */
  const struct patch_parameter *patch = patch_bank[patch_number];
  while (patch->param_id != MIDI_CC_UNSUPPORTED)
  {
    param_set_midi(patch->param_id, patch->value);
    patch++;
  }

  populate_cc_array(cc_param_map);
}
