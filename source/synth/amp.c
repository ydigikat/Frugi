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

#include "amp.h"


void amp_init(struct amp *amp, float fsr, float *samples, float *modulators)
{
	RTT_ASSERT(amp);
	RTT_ASSERT(samples);
	RTT_ASSERT(modulators);
	

	amp->samples = samples;
	amp->modulators = modulators;

	amp->fsr = fsr;
	
}

void amp_render(struct amp *amp, size_t block_size)
{
	RTT_ASSERT(amp);

	float *restrict ptr = amp->samples;
	float *restrict end = ptr + block_size;
	
	float mod_factor = 1.0f + (amp->mod_depth_param * amp->modulators[amp->mod_source_param]);
	float scale = amp->modulators[MOD_AMP_ENV_LEVEL] * amp->gain * mod_factor;
	

#ifdef DIM_OUTPUT
	scale *= 0.25f;
#endif	

#pragma GCC unroll 4
	while (ptr < end)
	{
		*ptr++ *= scale;
	}
}

void amp_update_params(struct amp *amp, float volume, float mod_source, float mod_depth)
{
	RTT_ASSERT(amp);
	amp->gain = volume;
	amp->mod_source_param = PARAM_TO_INT(mod_source, 0, MOD_MAX_SOURCE-1);
	amp->mod_depth_param = mod_depth;	
}
