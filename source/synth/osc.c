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
#include "osc.h"
#include "params.h"

/* Ranges to convert normalised parameters back to useful values */
#define OSC_OCTAVE_MIN (-2)
#define OSC_OCTAVE_MAX (2)
#define OSC_SEMI_MIN (-12)
#define OSC_SEMI_MAX (12)
#define OSC_CENTS_MIN (-50)
#define OSC_CENTS_MAX (50)
#define OSC_MOD_DEPTH_MIN (0.0f)
#define OSC_MOD_DEPTH_MAX (5.0f)

static void ugen_saw(struct osc *osc, float *samples, size_t block_size);
static void ugen_triangle(struct osc *osc, float *samples, size_t block_size);
static void ugen_pulse(struct osc *osc, float *samples, size_t block_size);

/* Adds a touch of analogue feel */
static inline float soft_saturation(float x)
{
#ifdef OSC_SOFT_SATURATION  
  return x * (1.0f - 0.3f * x * x);    
#else
  return x;    
#endif  
}

/* Sound generator jump table */
static void (*sound_generator[OSC_WAVE_MAX])(struct osc *osc, float *samples, size_t block_size) =
    {
        ugen_triangle,
        ugen_saw,
        ugen_pulse};

void osc_init(struct osc *osc, float fsr, float *samples, float *modulators, bool reset_buf)
{
  RTT_ASSERT(osc != NULL);
  RTT_ASSERT(samples != NULL);
  
  osc->reset_buf = reset_buf;
  osc->fsr = fsr;
  osc->phase = 0.0f;
  osc->inc = 0.0f;
  osc->pitch = 0.0f;
  osc->samples = samples;   
  osc->modulators = modulators;
  osc->wave_param = 0; 
  osc->pw_param = 0.5f;

  osc_reset(osc);
}


void osc_reset(struct osc *osc)
{
  RTT_ASSERT(osc != NULL);
  osc->phase = (osc->wave_param == OSC_TRIANGLE) ? 0.5f : 0;
}

static inline __attribute__((const)) float pitch_shift_multiplier(float semi_tones)
{
  if (semi_tones == 0)
    return 1.0f;

  return powf(2.0f, semi_tones / 12.0f);
}


void osc_render(struct osc *osc, size_t block_size)
{
  RTT_ASSERT(osc != NULL);
  RTT_ASSERT(osc->samples != NULL);

  if (osc->pitch == 0)
  {
    return;
  }
 
  osc->inc = osc->pitch * powf(2.0f, ((osc->mod_depth_param * osc->modulators[osc->mod_source_param]) + (float)(osc->octave_param * 12 + osc->semi_param) + (float)osc->cents_param * 0.01f) / 12.0f) / osc->fsr;

  sound_generator[osc->wave_param](osc, osc->samples, block_size);
}

/**
 * osc_note_off
 * \brief Silences the oscillator
 * \note Sets the pitch to zero, which causes the render function to
 *       skip audio generation until a new note-on event occurs.
 * \param osc Pointer to the oscillator instance
 */
void osc_note_on(struct osc *osc, float pitch)
{
  RTT_ASSERT(osc != NULL);

  osc_reset(osc);
  osc->pitch = pitch;
}

void osc_note_off(struct osc *osc)
{
  RTT_ASSERT(osc != NULL);
  osc->pitch = 0.0f;
}

void osc_update_params(struct osc *osc, float waveform, float octave, float semi, float cents,float level, float mod_source, float mod_depth, float pw)
{
  RTT_ASSERT(osc != NULL);
  
  osc->wave_param = PARAM_TO_INT(waveform, 0, OSC_WAVE_MAX-1);
  osc->octave_param = PARAM_TO_INT(octave, OSC_OCTAVE_MIN, OSC_OCTAVE_MAX);
  osc->semi_param = PARAM_TO_INT(semi, OSC_SEMI_MIN, OSC_SEMI_MAX);
  osc->cents_param = PARAM_TO_INT(cents, OSC_CENTS_MIN, OSC_CENTS_MAX);
  osc->mod_depth_param = PARAM_TO_LINEAR(mod_depth,OSC_MOD_DEPTH_MIN, OSC_MOD_DEPTH_MAX);
  osc->mod_source_param = PARAM_TO_INT(mod_source, MOD_LFO_TRIANGLE, MOD_MAX_SOURCE-1);
  osc->pw_param = pw;
  osc->level_param = level * 0.5f;  /* div by 2 as we have 2 oscillators*/
}

/**
 * ugen_saw
 * \brief Generates bandlimited sawtooth waveform using polynomial BLEP method
 * \note Implements a polynomal BLEP (Band-Limited Step) oscillator to reduce aliasing.
 *       The algorithm applies correction terms at discontinuities to create
 *       a high-quality digital sawtooth suitable for subtractive synthesis.
 * \param osc Pointer to the oscillator instance
 * \param samples Output samples to write or mix samples into
 * \param block_size Number of samples to generate
 */
static void ugen_saw(struct osc *osc, float *samples, size_t block_size)
{
  float *restrict ptr = samples;
  float *restrict end = samples + block_size;

  float inc = osc->inc;
  float level = osc->level_param;
  float phase = osc->phase;
  bool reset_buf = osc->reset_buf;  

#pragma GCC unroll 4
  while (ptr < end)
  {
    float saw = UNI_TO_BI(phase);

    /* Falling edge polynomial BLEP */
    if (phase > 1.0f - inc)
    {
      float t = (phase - 1.0f) / inc;
      saw += (t * t + 2.0f * t + 1.0f) * -1.0f;
    }
    else if (phase < inc)
    {
      /* Or the RH side */
      float t = phase / inc;
      saw += (2.0f * t - t * t - 1.0f) * -1.0f;
    }

    phase += inc;

    if (phase > 1.0f)
      phase -= 1.0f;

    float sample = soft_saturation(saw) * level * SAW_GAIN_SCALER;
    
    if (reset_buf)
      *ptr++ = sample;
    else
      *ptr++ += sample;
  }

  osc->phase = phase;
}


static void ugen_triangle(struct osc *osc, float *samples, size_t block_size)
{
  float *restrict ptr = samples;
  float *restrict end = samples + block_size;

  float phase = osc->phase;
  float inc = osc->inc;
  bool reset_buf = osc->reset_buf;
  bool level = osc->level_param;

#pragma GCC unroll 4
  while (ptr < end)
  {
    float sample = soft_saturation(2.0f * fabsf(2.0f * phase - 1.0f) - 1.0f) * level * TRI_GAIN_SCALER;

    if (reset_buf)
      *ptr++ = sample;
    else
      *ptr++ += sample;

    if (phase > 1.0f)
      phase -= 1.0f;

    phase += inc;    
  }  
  osc->phase = phase;
}

static void ugen_pulse(struct osc *osc, float *samples, size_t block_size)
{
  float *restrict ptr = samples;
  float *restrict end = samples + block_size;

  float phase = osc->phase;
  float inc = osc->inc;
  float pulse_width = osc->pw_param;
  float corr = (pulse_width < 0.5f) ? 1.0f / (1.0f - pulse_width) : 1.0f / pulse_width;
  float dc_offset = 1.0f - 2.0f * pulse_width;
  bool reset_buf = osc->reset_buf;
  bool level = osc->level_param;

#pragma GCC unroll 4
  while (ptr < end)
  {
    if (phase >= 1.0f)
      phase -= 1.0f;

    float phase_tmp = phase;
    float saw1 = UNI_TO_BI(phase_tmp);

    if (phase_tmp > 1.0f - inc)
    {
      float t = (phase_tmp - 1.0f) / inc;
      saw1 += (t * t + 2.0f * t + 1.0f) * -1.0f;
    }
    else if (phase_tmp < inc)
    {
      float t = phase_tmp / inc;
      saw1 += (2.0f * t - t * t - 1.0f) * -1.0f;
    }

    phase_tmp += pulse_width;

    if (phase_tmp >= 1.0f)
      phase_tmp -= 1.0f;

    float saw2 = UNI_TO_BI(phase_tmp);

    if (phase_tmp > 1.0f - inc)
    {
      float t = (phase_tmp - 1.0f) / inc;
      saw2 += (t * t + 2.0f * t + 1.0f) * -1.0f;
    }
    else if (phase_tmp < inc)
    {
      float t = phase_tmp / inc;
      saw2 += (2.0f * t - t * t - 1.0f) * -1.0f;
    }

    phase += inc;

    if (phase > 1.0f)
      phase -= 1.0f;

    float sample = (soft_saturation(saw1 - saw2 - dc_offset) * level) * corr * PULSE_GAIN_SCALER;

    if (reset_buf)
      *ptr++ = sample;
    else
      *ptr++ += sample;
  }

  osc->phase = phase;
}