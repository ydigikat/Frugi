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
#include "lfo.h"

#define LFO_PHASE_MAX 1.0f
#define LFO_PHASE_HALF 0.5f

static inline void lfo_triangle(struct lfo *lfo, float phase)
{
    lfo->modulators[MOD_LFO_TRIANGLE] = 4.0f * (float)fabsf(phase - 0.5f) - 1.0f;
}

static inline void lfo_saw(struct lfo *lfo, float phase)
{    
    lfo->modulators[MOD_LFO_SAW] = (2.0f * lfo->phase - 1.0f);
}

static inline void lfo_rev_saw(struct lfo *lfo, float phase)
{
    lfo->modulators[MOD_LFO_REV_SAW] = 1.0f - (2.0f * lfo->phase);
}

static inline void lfo_square(struct lfo *lfo, float phase)
{
    lfo->modulators[MOD_LFO_SQUARE] = lfo->phase > 0.5f ? -1.0f : 1.0f;
}

static inline void lfo_sample_and_hold(struct lfo *lfo, size_t block_size)
{
    static uint32_t rand_state = 2463534242; /* Seed for XORShift RNG */
    
    if (lfo->phase < lfo->prev_phase)
    {
        uint32_t random_int = xorshift32(&rand_state);
        lfo->sh_value = (float)random_int / (float)UINT32_MAX * 2.0f - 1.0f;
    }

    lfo->modulators[MOD_LFO_SANDH] = lfo->sh_value;

    lfo->prev_phase = lfo->phase;
}

void lfo_init(struct lfo *lfo, float fsr, float *modulators)
{
    RTT_ASSERT(lfo != NULL);
    RTT_ASSERT(modulators != NULL);

    lfo->fsr = fsr;
    lfo->modulators = modulators;
    lfo->hold_time = -1;
    lfo->sh_value = 0.0f;

    lfo_reset(lfo);
}

void lfo_reset(struct lfo *lfo)
{
    RTT_ASSERT(lfo != NULL);
    /* Not best for the triangle wave but not really noticeable on an LFO */
    lfo->phase = 0.0f;
}

void lfo_render(struct lfo *lfo, size_t block_size)
{
    RTT_ASSERT(lfo != NULL);
    RTT_ASSERT(lfo->modulators != NULL);

    float next_phase = fmodf(lfo->phase + (lfo->inc * (float)block_size), 1.0f);

    lfo_triangle(lfo, lfo->phase);
    lfo_saw(lfo, lfo->phase);
    lfo_rev_saw(lfo, lfo->phase);
    lfo_square(lfo, lfo->phase);
    lfo_sample_and_hold(lfo, block_size);

    lfo->phase = next_phase;
}

void lfo_update_params(struct lfo *lfo, float rate, float trigger_mode)
{
    RTT_ASSERT(lfo != NULL);

    lfo->rate_param = PARAM_TO_LINEAR(rate,LFO_MIN_RATE, LFO_MAX_RATE); 
    lfo->trigger_mode_param = trigger_mode;
    lfo->inc = lfo->rate_param / lfo->fsr;    
}

void lfo_note_on(struct lfo *lfo)
{
    RTT_ASSERT(lfo != NULL);
    
    if (lfo->trigger_mode_param == LFO_NOTE)
        lfo_reset(lfo);
}
