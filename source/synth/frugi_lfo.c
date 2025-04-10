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
#include "frugi_lfo.h"

#define LFO_PHASE_MAX 1.0f
#define LFO_PHASE_HALF 0.5f

static inline void lfo_triangle(struct frugi_lfo *lfo, float phase)
{
    lfo->lfo_values[LFO_TRIANGLE] = 4.0f * (float)FAST_FABS(phase - 0.5f) - 1.0f;
}

static inline void lfo_saw(struct frugi_lfo *lfo, float phase)
{    
    lfo->lfo_values[LFO_SAW] = (2.0f * lfo->phase - 1.0f);
}

static inline void lfo_rev_saw(struct frugi_lfo *lfo, float phase)
{
    lfo->lfo_values[LFO_REV_SAW] = 1.0f - (2.0f * lfo->phase);
}

static inline void lfo_square(struct frugi_lfo *lfo, float phase)
{
    lfo->lfo_values[LFO_SQUARE] = lfo->phase > 0.5f ? -1.0f : 1.0f;
}

static inline void lfo_sample_and_hold(struct frugi_lfo *lfo, size_t block_size)
{
    static uint32_t rand_state = 2463534242; /* Seed for XORShift RNG */

    if (lfo->phase < lfo->prev_phase)
    {

        uint32_t random_int = xorshift32(&rand_state);
        lfo->sh_value = (float)random_int / (float)UINT32_MAX * 2.0f - 1.0f;
    }

    lfo->lfo_values[LFO_SAMPLE_HOLD] = lfo->sh_value;

    lfo->prev_phase = lfo->phase;
}

void frugi_lfo_init(struct frugi_lfo *lfo, float fsr, float *lfo_values)
{
    RTT_ASSERT(lfo != NULL);
    RTT_ASSERT(lfo_values != NULL);

    lfo->fsr = fsr;
    lfo->lfo_values = lfo_values;
    lfo->hold_time = -1;
    lfo->sh_value = 0.0f;

    frugi_lfo_reset(lfo);
}

void frugi_lfo_reset(struct frugi_lfo *lfo)
{
    RTT_ASSERT(lfo != NULL);
    /* Not good for the triangle wave but not noticeable on an LFO */
    lfo->phase = 0.0f;
}

void frugi_lfo_render(struct frugi_lfo *lfo, size_t block_size)
{
    RTT_ASSERT(lfo != NULL);
    RTT_ASSERT(lfo->lfo_values != NULL);

    float next_phase = fmodf(lfo->phase + (lfo->inc * block_size), 1.0f);

    lfo_triangle(lfo, lfo->phase);
    lfo_saw(lfo, lfo->phase);
    lfo_rev_saw(lfo, lfo->phase);
    lfo_square(lfo, lfo->phase);
    lfo_sample_and_hold(lfo, block_size);

    lfo->phase = next_phase;
}

void frugi_lfo_update_params(struct frugi_lfo *lfo, float rate, float mode)
{
    RTT_ASSERT(lfo != NULL);

    lfo->rate = PARAM_TO_LINEAR(rate,LFO_MIN_RATE, LFO_MAX_RATE); 
    lfo->mode = mode;
    lfo->inc = lfo->rate / lfo->fsr;    
}

void frugi_lfo_note_on(struct frugi_lfo *lfo)
{
    RTT_ASSERT(lfo != NULL);
    
    if (lfo->mode == LFO_TRIGGER)
        frugi_lfo_reset(lfo);
}
