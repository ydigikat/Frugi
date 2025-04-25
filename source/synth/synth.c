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
#include "synth.h"

static struct voice *find_oldest_voice_to_steal(struct synth *synth);
static struct voice *find_oldest_voice_by_note(struct synth *synth, uint8_t note);
static void age_voices(struct synth *synth);
static void synth_note_on(struct synth *synth, uint8_t note, uint8_t velocity);
static void synth_note_off(struct synth *synth, uint8_t note);
static void synth_note_all_off(struct synth *synth);

/* TODO sustain override */
/* TODO portamento & bend */

void synth_init(struct synth *synth, float sample_rate, size_t block_size, uint8_t *midi_channel)
{
  RTT_ASSERT(synth);

  /* Allocate the memory for the voice output buffers */
  synth->voice_buffer_block = pvPortMalloc(MAX_VOICES * block_size * sizeof(float));
  memset(synth->voice_buffer_block, 0, MAX_VOICES * block_size * sizeof(float));

  /* Allocate the buffer for modulation values */
  synth->voice_modulators_block = pvPortMalloc(MAX_VOICES * sizeof(float) * MOD_MAX_SOURCE);
  memset(synth->voice_modulators_block, 0, sizeof(float) * MOD_MAX_SOURCE);

  /* Each voice gets a portion of the available audio headroom */
  synth->poly_attenuation = 1.0f / sqrtf(MAX_VOICES);

  /* Listen on all MIDI channels */
  *midi_channel = MIDI_OMNI;

  /* Load parameters into the DAE parameter store */
  load_factory_patch(0, synth->cc_to_param_map);

  /* Create a cached copy to share with modules */
  for (int i = 0; i < MAX_VOICES; i++)
  {
    synth->voice[i].id = i;
    voice_init(&synth->voice[i], synth->params, 
                synth->voice_buffer_block + (i * block_size), 
                synth->voice_modulators_block + (i * block_size), 
                sample_rate, block_size);
  }

  RTT_LOG("%s%sSynth & Voices  Initialised.\n", RTT_CTRL_CLEAR, RTT_CTRL_TEXT_BRIGHT_YELLOW);
}

/**
 * synth_render
 * \brief calls each voice in turn to accumulate the output audio
 * \param synth the synth instance
 * \param left the left sample buffer
 * \param right the right sample buffer
 * \param block_size the number of samples to render
 */
void synth_render(struct synth *synth, float *left, float *right, size_t block_size)
{
  RTT_ASSERT(synth);
  RTT_ASSERT(left);
  RTT_ASSERT(right);

  // DWT_INIT();
  // DWT_CLEAR();

  voice_render(&synth->voice[0]);
  voice_render(&synth->voice[1]);
  voice_render(&synth->voice[2]);
  voice_render(&synth->voice[3]);
  voice_render(&synth->voice[4]);
  voice_render(&synth->voice[5]);
  voice_render(&synth->voice[6]);
  voice_render(&synth->voice[7]);

  /*
   * Accumulate rendered samples into output buffers.
   *
   * While the buffers occupy a contiguous memory block (allocated by the synth), I found
   * using a single pointer to accumulate the entire block was slower than the individual
   * pointers below.  The copy takes around 40us which is acceptable (originally 76us 
   * using array notation/for loops).
   */

  const float scale = synth->poly_attenuation;

  float *restrict lp = left;
  float *restrict end = lp + block_size;
  float *restrict rp = right;

  float *restrict v0p = synth->voice[0].samples;
  float *restrict v1p = synth->voice[1].samples;
  float *restrict v2p = synth->voice[2].samples;
  float *restrict v3p = synth->voice[3].samples;
  float *restrict v4p = synth->voice[4].samples;
  float *restrict v5p = synth->voice[5].samples;
  float *restrict v6p = synth->voice[6].samples;
  float *restrict v7p = synth->voice[7].samples;

  while (lp < end)
  {
    *lp = (*v0p + *v1p + *v2p + *v3p + *v4p + *v5p + *v6p + *v7p) * scale;
    *rp = *lp; /* MONO so just copy */

    lp++;
    rp++;
    v0p++;
    v1p++;
    v2p++;
    v3p++;
    v4p++;
    v5p++;
    v6p++;
    v7p++;
  }

  // DWT_OUTPUT("Output");
}

/**
 * synth_midi_message
 * \brief MIDI message dispatcher, the bytes are already validated and make a complete MIDI message.
 * \note This will be called repeatedly for each message at the start of a block until the MIDI ring
 *       buffer is emptied.
 * \param synth the synthesiser
 * \param byte0 the status byte
 * \param byte1 MIDI data byte 1
 * \param byte2 MIDI data byte 2
 */
void synth_midi_message(struct synth *synth, uint8_t byte0, uint8_t byte1, uint8_t byte2)
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
    uint8_t id = synth->cc_to_param_map[byte1];
    if (id != MIDI_CC_UNSUPPORTED)
    {
      param_set_midi(id, byte2);
    }
    break;
  }
  default:
  }
}

/**
 * synth_update_params
 * \brief update the synth parameter cache
 * \note called by the DAE when it determines that something has changed one or more
 *       parameters in the central store it manages.  This is checked at the start of
 *       each audio block.
 * \param synth the synth instance.
 */
void synth_update_params(struct synth *synth)
{
  RTT_ASSERT(synth);

  /* Small parameter count so just refresh them all */
  for (int i = 0; i < SYNTH_PARAM_MAX - 1; i++)
  {
    /* Replace our cached shared copy with updated values from the DAE store */
    synth->params[i] = param_get(i);
  }

  /* Signal the voices that our cached parameters have changed, they need to update their modules etc */
  for (int i = 0; i < MAX_VOICES; i++)
  {
    voice_update_params(&synth->voice[i]);
  }
}

/*
 * Finds the oldest voice currently playing to steal and reuse for the most
 * recent note received.
 */
static inline struct voice *find_oldest_voice_to_steal(struct synth *synth)
{
  int8_t age = -1;
  struct voice *oldest_voice = NULL;

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

/*
 * Finds a voice playing the specified note for MIDI note-off handling.
 * The age comparison is maintained as a defensive measure, as normally
 * only one voice should be playing any given note. This ensures we
 * always find the correct voice even in edge cases where voice allocation
 * might temporarily have duplicates.
 */
static inline struct voice *find_oldest_voice_by_note(struct synth *synth, uint8_t note)
{
  int8_t age = -1;
  struct voice *oldest_voice = NULL;

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
static inline void age_voices(struct synth *synth)
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
static inline struct voice *find_free_voice(struct synth *synth)
{
  struct voice *free_voice = NULL;

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
 *  - A voice that is already sounding this note (retrigger)
 *  - A free voice
 *  - The oldest voice playing (to steal and reuse)
 */
static void synth_note_on(struct synth *synth, uint8_t note, uint8_t velocity)
{
  struct voice *voice = NULL;

  /* Check for already playing this note*/
  voice = find_oldest_voice_by_note(synth, note);
  if (voice)
  {
    voice_note_on(voice, note, velocity);
    return;
  }

  /* Find an available voice */
  voice = find_free_voice(synth);
  if (voice)
  {
    age_voices(synth);
    voice_note_on(voice, note, velocity);
    return;
  }

  /* Steal oldest voice */
  voice = find_oldest_voice_to_steal(synth);
  if (voice)
  {
    age_voices(synth);
    voice_note_on(voice, note, velocity);
  }
}

static void synth_note_off(struct synth *synth, uint8_t note)
{

  /* Find the voice playing the note */
  for (int i = 0; i < MAX_VOICES; i++)
  {
    struct voice *voice = find_oldest_voice_by_note(synth, note);
    if (voice)
    {
      voice_note_off(voice, note);
      break;
    }
  }
}

static void synth_note_all_off(struct synth *synth)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    voice_note_off(&synth->voice[i], synth->voice[i].current_note);
  }
}
