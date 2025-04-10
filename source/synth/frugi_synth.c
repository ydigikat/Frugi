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
#include "frugi_synth.h"

static struct frugi_voice *find_oldest_voice_to_steal(struct frugi_synth *synth);
static struct frugi_voice *find_oldest_voice_by_note(struct frugi_synth *synth, uint8_t note);
static void age_voices(struct frugi_synth *synth);
static void synth_note_on(struct frugi_synth *synth, uint8_t note, uint8_t velocity);
static void synth_note_off(struct frugi_synth *synth, uint8_t note);
static void synth_note_all_off(struct frugi_synth *synth);

/* TODO Portamento/Glissando support */
/* TODO Voice modes (poly, mono, unison)*/
/* TODO Global chorus */

/**
 * frugi_synth_init
 * \brief Initialises the synthesiser passing parameters from the DAE
 * \param synth the synthesiser instance
 * \param sample_rate the DAE sample rate
 * \param block_size the number of samples in each block (fixed)
 * \param midi_channel set by the synth to indicate what channel it is listening on 1-16 or OMNI
 */
void frugi_synth_init(struct frugi_synth *synth, float sample_rate, size_t block_size, uint8_t *midi_channel)
{

  RTT_ASSERT(synth);  

  /* Each voice gets a portion of the available audio headroom */
  synth->poly_attenuation = 1.0f / sqrtf(MAX_VOICES);

  /* Listen on all MIDI channels */
  *midi_channel = MIDI_OMNI;

  /* Load parameters into the DAE parameter store, create a cached copy and share that with the voices */
  load_factory_patch(0, synth->cc_to_param_map);
  for (int i = 0; i < MAX_VOICES; i++)
  {
    synth->voice[i].id = i;
    frugi_voice_init(&synth->voice[i], synth->params, sample_rate, block_size);
  }

  RTT_LOG("%s%sSynth & Voices  Initialised.\n", RTT_CTRL_CLEAR, RTT_CTRL_TEXT_BRIGHT_YELLOW);
}


/**
 * frugi_synth_render
 * \brief calls each voice in turn to render audio
 * \param synth the synth instance
 * \param left the left sample buffer
 * \param right the right sample buffer
 * \param block_size the number of samples to render
 */
void frugi_synth_render(struct frugi_synth *synth, float *left, float *right, size_t block_size)
{
  RTT_ASSERT(synth);
  RTT_ASSERT(left);
  RTT_ASSERT(right);

  frugi_voice_render(&synth->voice[0]);
  frugi_voice_render(&synth->voice[1]);

  frugi_voice_render(&synth->voice[2]);
#ifndef DEBUG
  frugi_voice_render(&synth->voice[3]);
  frugi_voice_render(&synth->voice[4]);
  frugi_voice_render(&synth->voice[5]);
#endif

  for (size_t i = 0; i < block_size; i++)
  {
    left[i] = (synth->voice[0].sample_buf[i]);
    left[i] += (synth->voice[1].sample_buf[i]);

    left[i] += (synth->voice[2].sample_buf[i]);
#ifndef DEBUG
    left[i] += (synth->voice[3].sample_buf[i]);

    left[i] += (synth->voice[4].sample_buf[i]);
    left[i] += (synth->voice[5].sample_buf[i]);
#endif

    left[i] *= synth->poly_attenuation;

    /* Mono voices so right is just a duplicate of left*/
    right[i] = left[i];
  }  
}


/**
 * frugi_synth_midi_message
 * \brief MIDI message dispatcher, the bytes are already validated and make a complete MIDI message.
 * \note This will be called repeatedly for each message at the start of a block until the MIDI ring 
 *       buffer is emptied.
 * \param synth the synthesiser
 * \param byte0 the status byte
 * \param byte1 MIDI data byte 1
 * \param byte2 MIDI data byte 2
 */
void frugi_synth_midi_message(struct frugi_synth *synth, uint8_t byte0, uint8_t byte1, uint8_t byte2)
{
  RTT_ASSERT(synth);

  switch (byte0)
  {
  case MIDI_STATUS_NOTE_OFF:
    synth_note_off(synth, byte1);
    break;

  case MIDI_STATUS_NOTE_ON:
  {
    if (byte2 > 0)
    {
      synth_note_on(synth, byte1, byte2);
    }
    else
    {
      /* Some devices send a note on with a value of 0 to indicate note off*/
      synth_note_off(synth, byte1);
    }
    break;
  }
  case MIDI_STATUS_CONTROL_CHANGE:
  {
    //RTT_LOG("CC:%d, VALUE:%d, PARAM:%d\n", byte1, byte2, synth->cc_to_param_map[byte1]);

    /*
     * This maps a CC message directly to the parameter that it is setting, these are defined
     * in the frugi_params module.
    */
    param_set_midi(synth->cc_to_param_map[byte1], byte2);
    break;
  }
  default:
  }
}

/**
 * frugi_synth_update_params
 * \brief update the synth parameter cache
 * \note called by the DAE when it determines that something has changed one or more
 *       parameters in the central store it manages.  This is checked at the start of
 *       each audio block.  
 * \param synth the synth instance.
 */
void frugi_synth_update_params(struct frugi_synth *synth)
{
  RTT_ASSERT(synth);
  
  /* Small parameter count so just refresh them all */
  for (int i = 0; i < FRUGI_PARAM_COUNT; i++)
  {
    /* Replace our cached shared copy with updated values from the DAE store */
    synth->params[i] = param_get(i);    
  }

  /* Handle any global parameters that the synth (rather than voice) manages*/

  /* TODO GLobal FX params will go here */


  /* Signal the voices that our cached parameters have changed, they need to update their modules etc */
  for (int i = 0; i < MAX_VOICES; i++)
  {
    frugi_voice_update_params(&synth->voice[i]);
  }
}


static inline struct frugi_voice *find_oldest_voice_to_steal(struct frugi_synth *synth)
{
  int8_t age = -1;
  struct frugi_voice *oldest_voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (!synth->voice[i].note_pending && synth->voice[i].note_on && synth->voice[i].age > age)
    {
      age = synth->voice[i].age;
      oldest_voice = &synth->voice[i];
    }
  }

  return oldest_voice;
}

/* TODO: Check why I wrote the test for age, only one note can be playing a voice so it will always be the oldest (only) one, perhaps
   to handle glide/portamento? */
static inline struct frugi_voice *find_oldest_voice_by_note(struct frugi_synth *synth, uint8_t note)
{
  int8_t age = -1;
  struct frugi_voice *oldest_voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++)
  {

    if (synth->voice[i].note_on && synth->voice[i].current_note == note && synth->voice[i].age > age)
    {
      age = synth->voice[i].age;
      oldest_voice = &synth->voice[i];
    }
  }

  return oldest_voice;
}

/** This ages all the voices by one generation */
static inline void age_voices(struct frugi_synth *synth)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].note_on)
    {
      synth->voice[i].age++;
    }
  }
}

/* Finds the first voice that is not playing */
static inline struct frugi_voice *find_free_voice(struct frugi_synth *synth)
{
  struct frugi_voice *free_voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (!synth->voice[i].note_on)
    {
      return &synth->voice[i];
    }
  }
  return free_voice;
}

/**
 * synth_note_on
 * \brief handles the synth note on signalling the voice to sound.
 * \note this looks in priority order for:
 *  - A voice that is already sounding this note
 *  - A free voice (silent)
 *  - The best (in this case oldest) voice to steal to play the note.
 */
static void synth_note_on(struct frugi_synth *synth, uint8_t note, uint8_t velocity)
{
  struct frugi_voice *voice = NULL;

  /* Check for already playing this note*/
  voice = find_oldest_voice_by_note(synth, note);
  if (voice)
  {
    frugi_voice_note_on(voice, note, velocity);
    return;
  }

  /* Find an available voice */
  voice = find_free_voice(synth);
  if (voice)
  {
    age_voices(synth);
    frugi_voice_note_on(voice, note, velocity);
    return;
  }

  /* Steal oldest voice */
  voice = find_oldest_voice_to_steal(synth);
  if (voice)
  {
    age_voices(synth);
    frugi_voice_note_on(voice, note, velocity);
  }
}

static void synth_note_off(struct frugi_synth *synth, uint8_t note)
{

  /* Find the voice playing the note */
  for (int i = 0; i < MAX_VOICES; i++)
  {
    struct frugi_voice *voice = find_oldest_voice_by_note(synth, note);
    if (voice)
    {
      frugi_voice_note_off(voice, note);
      break;
    }
  }
}

static void synth_note_all_off(struct frugi_synth *synth)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    frugi_voice_note_off(&synth->voice[i], synth->voice[i].current_note);
  }
}
