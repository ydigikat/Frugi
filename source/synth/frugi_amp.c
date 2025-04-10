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

#include "frugi_amp.h"

/* TODO Panning */

void frugi_amp_init(struct frugi_amp *amp, float fsr, float *sample_buf, float *env_level, float *lfo_values)
{
	RTT_ASSERT(amp);
	RTT_ASSERT(sample_buf);
	RTT_ASSERT(env_level);
	RTT_ASSERT(lfo_values);

	amp->sample_buf = sample_buf;
	amp->fsr = fsr;
	amp->env_level = env_level;
	amp->lfo_values = lfo_values;
}

void frugi_amp_render(struct frugi_amp *amp, size_t block_size)
{
	RTT_ASSERT(amp);

	float *restrict ptr = amp->sample_buf;
	float *restrict end = ptr + block_size;

	float mod_factor = 1.0f + (amp->lfo_depth * amp->lfo_values[amp->lfo_function]);
	float scale = *amp->env_level * amp->gain * mod_factor;

#pragma GCC unroll 4
	while (ptr < end)
	{
		*ptr++ *= scale;
	}
}

void frugi_amp_update_params(struct frugi_amp *amp, float volume, float pan, float mod_depth, float lfo_function)
{
	RTT_ASSERT(amp);

	amp->pan = pan;
	amp->gain = volume;
	amp->lfo_function = PARAM_TO_INT(lfo_function, 0, LFO_FUNCTION_COUNT);
	amp->lfo_depth = mod_depth;
}
