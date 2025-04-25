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

#include "filter.h"

/* Ranges for control mapping */

#define FILTER_CUT_MIN (80.0f)
#define FILTER_CUT_MAX (18000.0f)
#define FILTER_RES_MIN (0)
#define FILTER_RES_MAX (4.0f)
#define FILTER_MOD_NORMAL (4.0f)
#define FILTER_MOD_EXTREME (10.0f)
#define FILTER_SAT_MIN (0)
#define FILTER_SAT_MAX (5.0f)
#define FILTER_MOD_MAX (4.0f)

#define T (0.0000224f)
#define TWO_OVER_T (2.0f / T)
#define T_OVER_2 (T / 2.0f)

/* TODO  Filter tracking - MIDI note is now fed via a note_on()  but no alogrithm */

void filter_init(struct filter *filter, float fsr, float *samples, float *modulators)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(samples != NULL);
  RTT_ASSERT(modulators != NULL);  

  filter->fsr = fsr;
  filter->samples = samples;
  filter->modulators = modulators;

  filter->cutoff = FILTER_CUT_MAX;
  filter->alpha0 = 1.0f;
  filter->bass_comp = 1.0f;

  /* Init the cascade */
  filter->f1.alpha  = 1.0f;
  filter->f1.beta  = 0;
  filter->f1.z1  = 0;
  filter->f1.a0  = 1.0f;

  filter->f2.alpha = 1.0f;
  filter->f2.beta = 0;
  filter->f2.z1 = 0;
  filter->f2.a0 = 1.0f;

  filter->f3.alpha = 1.0f;
  filter->f3.beta = 0;
  filter->f3.z1 = 0;
  filter->f3.a0 = 1.0f;

  filter->f4.alpha = 1.0f;
  filter->f4.beta = 0;
  filter->f4.z1 = 0;
  filter->f4.a0 = 1.0f;

  filter_reset(filter);
}

void filter_reset(struct filter *filter)
{
  RTT_ASSERT(filter != NULL);

  filter->f1.z1 = 0;
  filter->f2.z1 = 0;
  filter->f3.z1 = 0;
  filter->f4.z1 = 0;
}

static void filter_calculate_coefficients(struct filter *filter)
{
  float lfo_mod = filter->modulators[filter->mod_source_param] * filter->mod_depth_param;  

  filter->cutoff = filter->cutoff_param * powf(FILTER_MOD_MAX, lfo_mod);

  /* Clamp modulation to filter range */
  if (filter->cutoff < FILTER_CUT_MIN)
  {
    filter->cutoff = FILTER_CUT_MAX;
  }
  else if (filter->cutoff > FILTER_CUT_MAX)
  {
    filter->cutoff = FILTER_CUT_MAX;
  }

  float g = (TWO_OVER_T * tanf((DAE_TWO_PI * filter->cutoff) * T_OVER_2)) * T_OVER_2;
  float G = g / (1.0f + g);

  filter->f1.alpha = G;
  filter->f2.alpha = G;
  filter->f3.alpha = G;
  filter->f4.alpha = G;

  filter->f1.beta = G * G * G / (1.0f + g);
  filter->f2.beta = G * G / (1.0f + g);
  filter->f3.beta = G / (1.0f + g);
  filter->f4.beta = 1.0f / (1.0f + g);

  float gamma = G * G * G * G;
  filter->alpha0 = 1.0f / (1.0f + filter->resonance_param * gamma);
}

typedef void (*output_func)(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4);
typedef void (*filter_func)(struct filter *, size_t, output_func select_taps);

struct jump_table_entry
{
  filter_func filter;
  output_func output;
};

static void output_lpf2(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = lpf1;
}

static void output_bpf2(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = lpf1 * 2.0f + lpf2 * -2.0f;
}

static void output_hpf2(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = U + lpf1 * -2.0f + lpf2;
}

static void output_lpf4(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = lpf4;
}

static void output_bpf4(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = lpf2 * 4.0f + lpf3 * -8.0f + lpf4 * 4.0f;
}

static void output_hpf4(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4)
{
  *output = U + lpf1 * -4.0f + lpf2 * 6.0f + lpf3 * -4.0f + lpf4;
}

static void internal_filter_render(struct filter * filter, size_t block_size, output_func select_taps)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(filter->samples != NULL);
  RTT_ASSERT(filter->modulators != NULL);
  RTT_ASSERT(select_taps != NULL);

  /* Stack cache variables */
  float alpha0 = filter->alpha0;
  float resonance = filter->resonance_param;
  float saturate = filter->saturation_param;

  float z1 = filter->f1.z1;
  float z2 = filter->f2.z1;
  float z3 = filter->f3.z1;
  float z4 = filter->f4.z1;

  float alpha1 = filter->f1.alpha;
  float alpha2 = filter->f2.alpha;
  float alpha3 = filter->f3.alpha;
  float alpha4 = filter->f4.alpha;

  float beta1 = filter->f1.beta;
  float beta2 = filter->f2.beta;
  float beta3 = filter->f3.beta;
  float beta4 = filter->f4.beta;

  float *restrict ptr = filter->samples;
  float *restrict end = ptr + block_size;

  while (ptr < end)
  {
    float sigma = beta1 * z1 + beta2 * z2 + beta3 * z3 + beta4 * z4;    
    float U=0;

#ifdef SATURATION_TANH_APPROX    
    U = *ptr - resonance * sigma * alpha0;  
    if (saturate > 0)
    {
      U *= saturate;
      U = U / (fabsf(2 * U) + 1.5f);
    }
#endif

#ifdef SATURATION_TANH
    U = *ptr - resonance * sigma * alpha0;  
    if (saturate > 0)
    {
      float x = U * saturate;
      float x2 = x * x;
      U = x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
#endif

#ifdef SATURATION_FEEDBACK
    float fb = resonance * sigma * alpha0;
    U = *ptr - fb;

    if (saturate > 0)
    {      
      fb = fb * 1.2f; 
      fb = fb / (1.0f + fabsf(0.5f * fb));
      U = *ptr - resonance * fb * alpha0;
    }
#endif    
    
    float vn = (U - z1) * alpha1;
    float lpf1 = vn + z1;
    z1 = vn + lpf1;

    vn = (lpf1 - z2) * alpha2;
    float lpf2 = vn + z2;
    z2 = vn + lpf2;

    vn = (lpf2 - z3) * alpha3;
    float lpf3 = vn + z3;
    z3 = vn + lpf3;

    vn = (lpf3 - z4) * alpha4;
    float lpf4 = vn + z4;
    z4 = vn + lpf4;
    
    select_taps(ptr++, U, lpf1, lpf2, lpf3, lpf4);
  }

  filter->f1.z1 = z1;
  filter->f2.z1 = z2;
  filter->f3.z1 = z3;
  filter->f4.z1 = z4;
}

static const struct jump_table_entry filter_funcs[] = {
    {internal_filter_render, output_lpf2},
    {internal_filter_render, output_bpf2},
    {internal_filter_render, output_hpf2},
    {internal_filter_render, output_lpf4},
    {internal_filter_render, output_bpf4},
    {internal_filter_render, output_hpf4}};

void filter_render(struct filter * filter, size_t block_size)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(filter->samples != NULL);
  RTT_ASSERT(filter->modulators != NULL);  

  filter_calculate_coefficients(filter);
  filter_funcs[filter->filter_type_param].filter(filter, block_size, filter_funcs[filter->filter_type_param].output);
}

void filter_update_params(struct filter *filter, float mode, float cutoff, float resonance,
                                float saturation, float mod_depth, float mod_source, float note_tracking)
{
  RTT_ASSERT(filter != NULL);

  filter->filter_type_param = PARAM_TO_INT(mode, 0, FILTER_TYPE_MAX-1);
  filter->cutoff_param = PARAM_TO_EXP(cutoff, FILTER_CUT_MIN, FILTER_CUT_MAX);  
  filter->resonance_param = PARAM_TO_LINEAR(resonance, FILTER_RES_MIN, FILTER_RES_MAX);
  filter->saturation_param = PARAM_TO_LINEAR(saturation, FILTER_SAT_MIN, FILTER_SAT_MAX);
  filter->note_tracking_param = note_tracking;
  
  filter->mod_depth_param = mod_depth;
  filter->mod_source_param = mod_source;  
}

void filter_note_on(struct filter *filter, uint8_t note)
{
  RTT_ASSERT(filter != NULL);

  if (filter->note_tracking_param)
  {
    
  }
}