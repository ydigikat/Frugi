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
#include "frugi_voice.h"

extern const float MIDI_FREQ_TABLE[128];

void frugi_voice_init(struct frugi_voice *voice, float *params, float fsr, size_t block_size)
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

  /* calls vApplicationMallocFailedHook() if fail and halt system */
  voice->sample_buf = pvPortMalloc(sizeof(float) * block_size);
  memset(voice->sample_buf, 0, voice->block_size * sizeof(float));

  voice->mod_env_level = 0;
  voice->amp_env_level = 0;

  /* Initialise signal chain modules */

  frugi_env_gen_init(&voice->amp_env, voice->fsr, voice->block_size, &voice->amp_env_level);
  frugi_env_gen_init(&voice->mod_env, voice->fsr, voice->block_size, &voice->mod_env_level);
  frugi_osc_init(&voice->osc1, voice->fsr, voice->sample_buf, voice->lfo_values, &voice->mod_env_level, 1);
  frugi_osc_init(&voice->osc2, voice->fsr, voice->sample_buf, voice->lfo_values, &voice->mod_env_level, 2);
  frugi_noise_init(&voice->noise, voice->fsr, voice->sample_buf);
  frugi_amp_init(&voice->amp, voice->fsr, voice->sample_buf, &voice->amp_env_level, voice->lfo_values);
  frugi_lfo_init(&voice->lfo, voice->fsr, voice->lfo_values);
  frugi_filter_init(&voice->filter, voice->fsr, voice->sample_buf, voice->lfo_values, &voice->mod_env_level);

  
}

void frugi_voice_reset(struct frugi_voice *voice)
{
  RTT_ASSERT(voice != NULL);

  frugi_lfo_reset(&voice->lfo);
  frugi_osc_reset(&voice->osc1);
  frugi_osc_reset(&voice->osc2);
  frugi_noise_reset(&voice->noise);
  frugi_env_gen_reset(&voice->amp_env);
  frugi_env_gen_reset(&voice->mod_env);
  frugi_filter_reset(&voice->filter);
}

void frugi_voice_render(struct frugi_voice *voice)
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
    frugi_lfo_reset(&voice->lfo);
    frugi_osc_reset(&voice->osc1);
    frugi_osc_reset(&voice->osc2);
    frugi_noise_reset(&voice->noise);
    frugi_env_gen_reset(&voice->amp_env);
    frugi_env_gen_reset(&voice->mod_env);
    frugi_filter_reset(&voice->filter);
    memset(voice->sample_buf, 0, voice->block_size * sizeof(float));
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
    frugi_lfo_note_on(&voice->lfo);
    frugi_osc_note_on(&voice->osc1, voice->current_pitch);
    frugi_osc_note_on(&voice->osc2, voice->current_pitch);
    frugi_noise_note_on(&voice->noise);
    frugi_filter_note_on(&voice->filter, voice->current_note);
    frugi_env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    frugi_env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
  }

  //DWT_INIT();
  //DWT_CLEAR();
  frugi_osc_render(&voice->osc1, voice->block_size);
  //DWT_OUTPUT("Osc1 render");DWT_CLEAR();
  frugi_osc_render(&voice->osc2, voice->block_size);
  //DWT_OUTPUT("Osc2 render");DWT_CLEAR();
  frugi_noise_render(&voice->noise, voice->block_size);
  //DWT_OUTPUT("Noise render");DWT_CLEAR();
  frugi_env_gen_render(&voice->amp_env, voice->block_size);
  //DWT_OUTPUT("Amp env render");DWT_CLEAR();
  // frugi_env_gen_render(&voice->mod_env, voice->block_size);
  //DWT_OUTPUT("Mod env render");DWT_CLEAR();
  //DWT_CLEAR();
  frugi_filter_render(&voice->filter, voice->block_size);
  //DWT_OUTPUT("Filter render");DWT_CLEAR();
  frugi_amp_render(&voice->amp, voice->block_size);
  //DWT_OUTPUT("Amp render");DWT_CLEAR();
  frugi_lfo_render(&voice->lfo, voice->block_size);
  //DWT_OUTPUT("LFO render");DWT_CLEAR();  
}

void frugi_voice_note_on(struct frugi_voice *voice, uint8_t midi_note, uint8_t midi_velocity)
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
    frugi_env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    frugi_env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
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
    frugi_lfo_note_on(&voice->lfo);
    frugi_osc_note_on(&voice->osc1, voice->current_pitch);
    frugi_osc_note_on(&voice->osc2, voice->current_pitch);
    frugi_noise_note_on(&voice->noise);
    frugi_filter_note_on(&voice->filter, voice->current_note);
    frugi_env_gen_note_on(&voice->amp_env, voice->current_note, voice->current_velocity);
    frugi_env_gen_note_on(&voice->mod_env, voice->current_note, voice->current_velocity);
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
    frugi_env_gen_rtz(&voice->amp_env);
    frugi_env_gen_rtz(&voice->mod_env);
  }
}

void frugi_voice_note_off(struct frugi_voice *voice, uint8_t midi_note)
{
  RTT_ASSERT(voice != NULL);

  if (voice->note_on)
  {
    frugi_env_gen_note_off(&voice->amp_env);
    frugi_env_gen_note_off(&voice->mod_env);
  }
}

/*
 * Voice forwards updates so modules are not coupled to the structure
 * of Frugi's parameters.
 */
void frugi_voice_update_params(struct frugi_voice *voice)
{
  RTT_ASSERT(voice != NULL);
  
  frugi_osc_update_params(&voice->osc1,
                          voice->params[FRUGI_OSC1_WAVE],
                          voice->params[FRUGI_OSC1_OCTAVE],
                          voice->params[FRUGI_OSC1_SEMI],
                          voice->params[FRUGI_OSC1_CENTS],
                          voice->params[FRUGI_OSC1_PULSE_WIDTH],
                          voice->params[FRUGI_OSC1_LEVEL],
                          voice->params[FRUGI_OSC1_LFO_DEPTH],
                          voice->params[FRUGI_OSC1_LFO_FUNCTION],
                          voice->params[FRUGI_OSC1_ENV_DEPTH]);

  frugi_osc_update_params(&voice->osc2,
                          voice->params[FRUGI_OSC2_WAVE],
                          voice->params[FRUGI_OSC2_OCTAVE],
                          voice->params[FRUGI_OSC2_SEMI],
                          voice->params[FRUGI_OSC2_CENTS],
                          voice->params[FRUGI_OSC2_PULSE_WIDTH],
                          voice->params[FRUGI_OSC2_LEVEL],
                          voice->params[FRUGI_OSC2_LFO_DEPTH],
                          voice->params[FRUGI_OSC2_LFO_FUNCTION],
                          voice->params[FRUGI_OSC2_ENV_DEPTH]);

  frugi_noise_update_params(&voice->noise,
                            voice->params[FRUGI_NOISE_LEVEL],
                            voice->params[FRUGI_NOISE_TYPE]);

  frugi_lfo_update_params(&voice->lfo,
                          voice->params[FRUGI_LFO_RATE],
                          voice->params[FRUGI_LFO_MODE]);

  frugi_env_gen_update_params(&voice->amp_env,
                              voice->params[FRUGI_AMP_ENV_ATTACK],
                              voice->params[FRUGI_AMP_ENV_DECAY],
                              voice->params[FRUGI_AMP_ENV_SUSTAIN],
                              voice->params[FRUGI_AMP_ENV_RELEASE],
                              ENV_NORMAL,
                              voice->params[FRUGI_AMP_ENV_NOTE_TRACK],
                              voice->params[FRUGI_AMP_ENV_VEL_SENS]);

  frugi_filter_update_params(&voice->filter,
                             voice->params[FRUGI_FILTER_MODE],
                             voice->params[FRUGI_FILTER_CUTOFF],
                             voice->params[FRUGI_FILTER_RESONANCE],
                             voice->params[FRUGI_FILTER_SATURATION],
                             voice->params[FRUGI_FILTER_LFO_DEPTH],
                             voice->params[FRUGI_FILTER_LFO_FUNCTION],
                             voice->params[FRUGI_FILTER_ENV_AMOUNT],
                             voice->params[FRUGI_FILTER_LFO_RANGE],
                             voice->params[FRUGI_FILTER_NOTE_TRACK]);

  frugi_env_gen_update_params(&voice->mod_env,
                              voice->params[FRUGI_MOD_ENV_ATTACK],
                              voice->params[FRUGI_MOD_ENV_DECAY],
                              voice->params[FRUGI_MOD_ENV_SUSTAIN],
                              voice->params[FRUGI_MOD_ENV_RELEASE],
                              voice->params[FRUGI_MOD_ENV_MODE],
                              voice->params[FRUGI_MOD_ENV_NOTE_TRACK],
                              voice->params[FRUGI_MOD_ENV_VEL_SENS]);

  frugi_amp_update_params(&voice->amp,
                          voice->params[FRUGI_AMP_VOLUME],
                          voice->params[FRUGI_AMP_PAN],
                          voice->params[FRUGI_AMP_LFO_DEPTH],
                          voice->params[FRUGI_AMP_LFO_FUNCTION]);
  
}