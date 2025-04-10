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
#include "frugi_osc.h"
#include "frugi_params.h"

/* Ranges to convert normalised parameters back to useful values */
#define OSC_OCTAVE_MIN (-2)
#define OSC_OCTAVE_MAX (2)
#define OSC_SEMI_MIN (-11)
#define OSC_SEMI_MAX (11)
#define OSC_CENTS_MIN (-99)
#define OSC_CENTS_MAX (99)
#define OSC_PW_MIN (0.02f)
#define OSC_PW_MAX (0.98f)

/* TODO Pulse Width Modulation */


/**
 * soft_saturation
 * \brief Applies a polynomial-based soft clipping to the input signal
 * \note Uses a cubic waveshaper with the formula x * (1.0 - 0.3 * x²) to create
 *       a smooth saturation effect without harsh clipping artifacts.
 * \param x Input signal value to be saturated
 * \return Saturated output signal value
 */
static inline float soft_saturation(float x)
{
  return x * (1.0f - 0.3f * x * x);
  
}

/**
 * phase_jitter
 * \brief Generates small random phase variations to reduce aliasing and add analog character
 * \note can be enabled to add subtle random variations of ±0.0001 to the oscillator phase increment.
 * \return Random phase offset value
 */
static inline float phase_jitter(void)
{
  // return ((float)rand() / RAND_MAX) * 0.0002f - (0.0002f / 2.0f);
  return 0;
}

static void ugen_saw(struct frugi_osc *osc, float *io_buffer, size_t block_size);
static void ugen_pulse(struct frugi_osc *osc, float *sample_bufferm, size_t block_size);
static void ugen_triangle(struct frugi_osc *osc, float *io_buffer, size_t block_size);

/* Sound generator jump table */
static void (*sound_generator[3])(struct frugi_osc *osc, float *buffer, size_t block_size) =
    {
        ugen_triangle,
        ugen_saw,
        ugen_pulse};

/**
 * frugi_osc_init
 * \brief Initializes a new oscillator instance with default settings
 * \param osc Pointer to the oscillator instance to initialize
 * \param fsr Sample rate in Hz (e.g., 44100, 48000)
 * \param sample_buf Pointer to the output buffer for generated audio
 * \param lfo_values Pointer to array of LFO values for modulation
 * \param env_level Pointer to envelope value for modulation
 * \param id Unique identifier for the oscillator (1=first oscillator)
 */
void frugi_osc_init(struct frugi_osc *osc, float fsr, float *sample_buf, float *lfo_values, float *env_level, uint8_t id)
{
  RTT_ASSERT(osc != NULL);
  RTT_ASSERT(sample_buf != NULL);
  RTT_ASSERT(lfo_values != NULL);
  RTT_ASSERT(env_level != NULL);

  osc->id = id;
  osc->fsr = fsr;
  osc->phase = 0.0f;
  osc->inc = 0.0f;
  osc->pitch = 0.0f;
  osc->sample_buf = sample_buf;
  osc->lfo_values = lfo_values;
  osc->env_level = env_level;

  frugi_osc_reset(osc);
}

/**
 * frugi_osc_reset
 * \brief Resets the oscillator phase to an appropriate starting position
 * \note Sets phase to 0.0 for sawtooth and pulse waves, or 0.5 for triangle waves
 *       to ensure the waveforms start at zero amplitude.
 * \param osc Pointer to the oscillator instance to reset
 */
void frugi_osc_reset(struct frugi_osc *osc)
{
  RTT_ASSERT(osc != NULL);
  osc->phase = (osc->wave == OSC_TRIANGLE) ? 0.5f : 0;
}

/**
 * frugi_osc_render
 * \brief Calculates phase increment and generates one block of audio
 * \note Combines base pitch with modulation from LFO, envelope, and tuning parameters
 *       to calculate the final phase increment, then calls the appropriate waveform
 *       generator function. The phase increment formula implements equal-tempered
 *       tuning with semitone/cent accuracy.
 * \param osc Pointer to the oscillator instance
 * \param block_size Number of samples to generate
 */
void frugi_osc_render(struct frugi_osc *osc, size_t block_size)
{
  RTT_ASSERT(osc != NULL);
  RTT_ASSERT(osc->sample_buf != NULL);
  RTT_ASSERT(osc->lfo_values != NULL);
  RTT_ASSERT(osc->env_level != NULL);

  if (osc->pitch == 0)
  {
    return;
  }

  osc->inc = osc->pitch * fast_pow(2.0f, ((osc->lfo_depth * osc->lfo_values[osc->lfo_function]) + (osc->env_depth * *osc->env_level) + (osc->octave * 12.0f) + osc->semi + (osc->cents * 0.01f)) / 12.0f) / osc->fsr;

  sound_generator[osc->wave](osc, osc->sample_buf, block_size);
}

/**
 * frugi_osc_note_off
 * \brief Silences the oscillator
 * \note Sets the pitch to zero, which causes the render function to
 *       skip audio generation until a new note-on event occurs.
 * \param osc Pointer to the oscillator instance
 */
void frugi_osc_note_on(struct frugi_osc *osc, float pitch)
{
  RTT_ASSERT(osc != NULL);

  frugi_osc_reset(osc);
  osc->pitch = pitch;
}

void frugi_osc_note_off(struct frugi_osc *osc)
{
  RTT_ASSERT(osc != NULL);
  osc->pitch = 0.0f;
}

/**
 * frugi_osc_update_params
 * \brief Updates all oscillator parameters
 * \note All input parameters are in normalized range (0.0-1.0) and are
 *       internally converted to appropriate units for oscillator control.
 * \param osc Pointer to the oscillator instance
 * \param waveform Oscillator waveform selection (0=triangle, 1=saw, 2=pulse)
 * \param octave Octave tuning (-2 to +2 octaves from base pitch)
 * \param semi Semitone tuning (-11 to +11 semitones)
 * \param cents Fine tuning (-99 to +99 cents, 100 cents = 1 semitone)
 * \param pw Pulse width for pulse waveform (0.02 to 0.98, 0.5 = square wave)
 * \param level Output amplitude (0.0-1.0, scaled by 0.3 to balance with other oscillators)
 * \param depth LFO modulation depth (pitch modulation amount)
 * \param function LFO waveform selection for modulation
 * \param env_depth Envelope modulation depth (typically for pitch envelope)
 */
void frugi_osc_update_params(struct frugi_osc *osc, float waveform, float octave, float semi, float cents,
                             float pw, float level, float depth, float function, float env_depth)
{
  RTT_ASSERT(osc != NULL);

  osc->wave = PARAM_TO_INT(waveform, 0, 2);
  osc->octave = PARAM_TO_INT(octave, OSC_OCTAVE_MIN, OSC_OCTAVE_MAX);
  osc->semi = PARAM_TO_LINEAR(semi, OSC_SEMI_MIN, OSC_SEMI_MAX);
  osc->cents = PARAM_TO_LINEAR(cents, OSC_CENTS_MIN, OSC_CENTS_MAX);
  osc->pw = PARAM_TO_LINEAR(pw, OSC_PW_MIN, OSC_PW_MAX);

  osc->level = level * 0.3f; /* 0.3f to allow for the 2 oscillators + noise */
  osc->lfo_depth = depth;
  osc->lfo_function = function;
  osc->env_depth = env_depth;
}

/**
 * ugen_saw
 * \brief Generates bandlimited sawtooth waveform using polynomial BLEP method
 * \note Implements a BLEP (Band-Limited Step) oscillator to reduce aliasing.
 *       The algorithm applies correction terms at discontinuities to create
 *       a high-quality digital sawtooth suitable for subtractive synthesis.
 * \param osc Pointer to the oscillator instance
 * \param sample_buf Output buffer to write or mix samples into
 * \param block_size Number of samples to generate
 */
static void ugen_saw(struct frugi_osc *osc, float *sample_buf, size_t block_size)
{
  float *restrict ptr = sample_buf;
  float *restrict end = sample_buf + block_size;

  float phase = osc->phase;
  float inc = osc->inc + phase_jitter();
  float pw = osc->pw;

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

    float sample = soft_saturation(saw) * osc->level;

    if (osc->id == 1)
      *ptr++ = sample;
    else
      *ptr++ += sample;
  }

  osc->phase = phase;
}

/**
 * ugen_pulse
 * \brief Generates bandlimited pulse/square waveform with variable pulse width
 * \note Implements the pulse wave as the difference between two sawtooth waves
 *       offset by the pulse width. Uses BLEP correction at discontinuities.
 *       Compensates for DC offset based on pulse width to maintain consistent volume.
 * \param osc Pointer to the oscillator instance
 * \param sample_buf Output buffer to write or mix samples into
 * \param block_size Number of samples to generate
 */
static void ugen_pulse(struct frugi_osc *osc, float *sample_buf, size_t block_size)
{
  float *restrict ptr = sample_buf;
  float *restrict end = sample_buf + block_size;

  float phase = osc->phase;
  float inc = osc->inc + phase_jitter();
  float pulse_width = osc->pw;
  float corr = (pulse_width < 0.5f) ? 1.0f / (1.0f - pulse_width) : 1.0f / pulse_width;
  float dc_offset = 1.0f - 2.0f * pulse_width;

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

    float sample = (soft_saturation(saw1 - saw2 - dc_offset) * osc->level);

    if (osc->id == 1)
      *ptr++ = sample;
    else
      *ptr++ += sample;
  }

  osc->phase = phase;
}

/**
 * ugen_triangle
 * \brief Generates triangle waveform
 * \note Creates a triangle wave from absolute value of phase with transformations.
 *       Triangle waves naturally have less high frequency content and therefore
 *       require less anti-aliasing treatment than saw or pulse waves.
 * \param osc Pointer to the oscillator instance
 * \param sample_buf Output buffer to write or mix samples into
 * \param block_size Number of samples to generate
 */
static void ugen_triangle(struct frugi_osc *osc, float *sample_buf, size_t block_size)
{
  float *restrict ptr = sample_buf;
  float *restrict end = sample_buf + block_size;
  float phase = osc->phase;
  float inc = osc->inc;

  float jitter = phase_jitter();

#pragma GCC unroll 4
  while (ptr < end)
  {

    float sample = soft_saturation(2.0f * FAST_FABS(2.0f * phase - 1.0f) - 1.0f) * osc->level;

    if (osc->id == 1)
      *ptr++ = sample;
    else
      *ptr++ += sample;

    if (phase > 1.0f)
      phase -= 1.0f;

    ptr++;
  }

  osc->phase = phase;
}
