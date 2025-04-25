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
#include "voice.h"

extern const float MIDI_FREQ_TABLE[128];

void voice_init(struct voice *voice, float *params, float *samples, float *modulators, float fsr, size_t block_size)
{
  RTT_ASSERT(voice != NULL);
  RTT_ASSERT(params != NULL);
  RTT_ASSERT(fsr > 0.0f);
  RTT_ASSERT(block_size > 0);

  voice->params = params;
  voice->fsr = fsr;
  voice->block_size = block_size;

  voice->note_on = false;
  voice->note_pending = false;
  voice->current_note = 0;
  voice->pending_note = 0;
  voice->current_velocity = 0;
  voice->pending_velocity = 0;
  voice->current_pitch = 0.0f;
  voice->pending_pitch = 0.0f;
  voice->block_size = block_size;
  voice->samples = samples;
  voice->modulators = modulators;

  /* Initialise modulators */
  env_gen_init(&voice->amp_env, voice->fsr, voice->block_size, &voice->modulators[MOD_AMP_ENV_LEVEL]);
  env_gen_init(&voice->mod_env, voice->fsr, voice->block_size, &voice->modulators[MOD_ENV_LEVEL]);
  lfo_init(&voice->lfo, voice->fsr, voice->modulators);

  /* Initialise audio signal chain */
  osc_init(&voice->osc1, voice->fsr, voice->samples, voice->modulators, true);
  osc_init(&voice->osc2, voice->fsr, voice->samples, voice->modulators, false);
  amp_init(&voice->amp, voice->fsr, voice->samples, voice->modulators);
  filter_init(&voice->filter, voice->fsr, voice->samples, voice->modulators);
}

void voice_reset(struct voice *voice)
{
  RTT_ASSERT(voice != NULL);
  osc_reset(&voice->osc1);
  osc_reset(&voice->osc2);
  env_gen_reset(&voice->amp_env);
  lfo_reset(&voice->lfo);
  filter_reset(&voice->filter);
}

void voice_render(struct voice *voice)
{
  RTT_ASSERT(voice != NULL);

  if (!voice->note_on)
  {
    return;
  }

  /* Handle simple case of a note going of with no pending note */
  if (voice->amp_env.state == ENV_OFF && !voice->note_pending)
  {
    voice->note_on = false;
    lfo_reset(&voice->lfo);
    osc_reset(&voice->osc1);
    osc_reset(&voice->osc2);
    env_gen_reset(&voice->amp_env);
    env_gen_reset(&voice->mod_env);
    filter_reset(&voice->filter);
    // memset(voice->samples, 0, voice->block_size * sizeof(float));
    return;
  }

  /* Handle the voice steal case */
  if (voice->amp_env.state == ENV_OFF && voice->note_pending)
  {
    /* Switch to the new note */
    voice->current_note = voice->pending_note;
    voice->current_velocity = voice->pending_velocity;
    voice->current_pitch = voice->pending_pitch;
    voice->current_vel_factor = voice->pending_vel_factor;
    voice->note_pending = false;
    voice->note_on = true;

    voice->age = 0;

    /* We need to start the new note sounding in this case */
    lfo_note_on(&voice->lfo);
    osc_note_on(&voice->osc1, voice->current_pitch);
    osc_note_on(&voice->osc2, voice->current_pitch);
    filter_note_on(&voice->filter, voice->current_note);
    env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
  }

  // DWT_INIT();
  // DWT_CLEAR();

  lfo_render(&voice->lfo, voice->block_size);
  // DWT_OUTPUT("LFO");
  // DWT_CLEAR();

  osc_render(&voice->osc1, voice->block_size);
  // DWT_OUTPUT("OSC1");
  // DWT_CLEAR();

  osc_render(&voice->osc2, voice->block_size);
  // DWT_OUTPUT("OSC2");
  // DWT_CLEAR();

  env_gen_render(&voice->amp_env, voice->block_size);
  // DWT_OUTPUT("ENV1");
  // DWT_CLEAR();

  env_gen_render(&voice->mod_env, voice->block_size);
  // DWT_OUTPUT("ENV2");
  // DWT_CLEAR();

  filter_render(&voice->filter, voice->block_size);
  // DWT_OUTPUT("FILTER");
  // DWT_CLEAR();

  amp_render(&voice->amp, voice->block_size);
  // DWT_OUTPUT("AMP");
  // DWT_CLEAR();
}

void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity)
{
  RTT_ASSERT(voice != NULL);

  /* Check for retriggers of the same note */
  if (voice->note_pending && midi_note == voice->pending_note)
  {
    /* We're in the middle of a voice steal and the player retriggered the same note, ignore it */
    return;
  }

  if (!voice->note_pending && voice->note_on && voice->current_note == midi_note)
  {
    /* We're just playing the note and the user retriggered it, jump to ATTACK phase*/
    env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
    return;
  }

  /* Handle a change in the note */

  if (!voice->note_on)
  {
    /* Easy case, unused voice so just allocate it and done. */
    voice->current_note = midi_note;
    voice->current_velocity = midi_velocity;
    voice->current_pitch = MIDI_FREQ_TABLE[midi_note];
    /* Linear velocity factor*/
    voice->current_vel_factor = (midi_velocity / 127.0f) * 0.9f + 0.1f;
    voice->note_pending = false;
    voice->note_on = true;
    voice->age = 0;

    /* New note on */
    lfo_note_on(&voice->lfo);
    osc_note_on(&voice->osc1, voice->current_pitch);
    osc_note_on(&voice->osc2, voice->current_pitch);
    filter_note_on(&voice->filter, voice->current_note);
    env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
    return;
  }
  else
  {
    /*
     * This is a voice steal.  We need to store the note for the moment as the envelope has to complete
     * before we can apply it.  The RTZ call will end the envelope quickly (but not immediately) and then
     * in this case the voice_render() handles the start of the new note.
     */
    voice->note_pending = true;
    voice->pending_note = midi_note;
    voice->pending_velocity = midi_velocity;
    voice->pending_pitch = MIDI_FREQ_TABLE[midi_note];
    voice->pending_vel_factor = (midi_velocity / 127.0f) * 0.9f + 0.1f;
    env_gen_rtz(&voice->amp_env);
  }
}

void voice_note_off(struct voice *voice, uint8_t midi_note)
{
  RTT_ASSERT(voice != NULL);

  if (voice->note_on)
  {
    env_gen_note_off(&voice->amp_env);
    env_gen_note_off(&voice->mod_env);
  }
}

/*
 * Voice forwards updates so modules are not coupled to the structure
 * of Frugi's parameters.
 */
void voice_update_params(struct voice *voice)
{
  RTT_ASSERT(voice != NULL);

  osc_update_params(&voice->osc1,
                    voice->params[OSC1_WAVE],
                    voice->params[OSC1_OCTAVE],
                    voice->params[OSC1_SEMI],
                    voice->params[OSC1_CENTS],
                    voice->params[OSC1_LEVEL],
                    voice->params[OSC1_MOD_SOURCE],
                    voice->params[OSC1_MOD_DEPTH],
                    voice->params[OSC1_PW]);

  osc_update_params(&voice->osc2,
                    voice->params[OSC2_WAVE],
                    voice->params[OSC2_OCTAVE],
                    voice->params[OSC2_SEMI],
                    voice->params[OSC2_CENTS],
                    voice->params[OSC2_LEVEL],
                    voice->params[OSC2_MOD_SOURCE],
                    voice->params[OSC2_MOD_DEPTH],
                    voice->params[OSC2_PW]);

  env_gen_update_params(&voice->amp_env,
                        voice->params[AMP_ENV_ATTACK],
                        voice->params[AMP_ENV_DECAY],
                        voice->params[AMP_ENV_SUSTAIN],
                        voice->params[AMP_ENV_RELEASE],
                        ENV_NORMAL,
                        voice->params[AMP_ENV_NOTE_TRACK],
                        voice->params[AMP_ENV_VEL_SENS]);

  env_gen_update_params(&voice->mod_env,
                        voice->params[MOD_ENV_ATTACK],
                        voice->params[MOD_ENV_DECAY],
                        voice->params[MOD_ENV_SUSTAIN],
                        voice->params[MOD_ENV_RELEASE],
                        voice->params[MOD_ENV_MODE],
                        voice->params[MOD_ENV_NOTE_TRACK],
                        voice->params[MOD_ENV_VEL_SENS]);

  amp_update_params(&voice->amp,
                    voice->params[AMP_VOLUME],
                    voice->params[AMP_MOD_SOURCE],
                    voice->params[AMP_MOD_DEPTH]);

  lfo_update_params(&voice->lfo,
                    voice->params[LFO_RATE],
                    voice->params[LFO_TRIGGER_MODE]);

  filter_update_params(&voice->filter,
                       voice->params[FILTER_TYPE],
                       voice->params[FILTER_CUTOFF],
                       voice->params[FILTER_RESONANCE],
                       voice->params[FILTER_SATURATION],
                       voice->params[FILTER_MOD_DEPTH],
                       voice->params[FILTER_MOD_SOURCE],
                       voice->params[FILTER_NOTE_TRACK]);
}