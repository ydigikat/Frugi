#include "frugi_filter.h"

/* Ranges for control mapping */

#define FILTER_CUT_MIN (80.0f)
#define FILTER_CUT_MAX (18000.0f)
#define FILTER_RES_MIN (0)
#define FILTER_RES_MAX (4.0f)
#define FILTER_MOD_NORMAL (4.0f)
#define FILTER_MOD_EXTREME (10.0f)
#define FILTER_SAT_MIN (0)
#define FILTER_SAT_MAX (5.0f)

#define T (0.0000224f)
#define TWO_OVER_T (2.0f / T)
#define T_OVER_2 (T / 2.0f)

/* TODO  Filter tracking - MIDI note is now fed via a note_on()  but no alogrithm */
/* FIXME Self-resonance/saturation damping is crude and unreliable, compare against PC version */

void frugi_filter_init(struct frugi_filter *filter, float fsr, float *sample_buf, float *lfo_values, float *env_level)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(sample_buf != NULL);
  RTT_ASSERT(lfo_values != NULL);
  RTT_ASSERT(env_level != NULL);

  filter->fsr = fsr;
  filter->sample_buf = sample_buf;
  filter->lfo_values = lfo_values;
  filter->env_level = env_level;

  filter->actual_cutoff = FILTER_CUT_MAX;
  filter->alpha0 = 1.0f;
  filter->bass_comp = 1.0f;

  /* Init the cascade */
  filter->f1.alpha = filter->f2.alpha = filter->f3.alpha = filter->f4.alpha = 1.0f;
  filter->f1.beta = filter->f2.beta = filter->f3.beta = filter->f4.beta = 0;
  filter->f1.z1 = filter->f2.z1 = filter->f3.z1 = filter->f4.z1 = 0;
  filter->f1.a0 = filter->f2.a0 = filter->f3.a0 = filter->f4.a0 = 1.0f;

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

  frugi_filter_reset(filter);
}

void frugi_filter_reset(struct frugi_filter *filter)
{
  RTT_ASSERT(filter != NULL);

  filter->f1.z1 = 0;
  filter->f2.z1 = 0;
  filter->f3.z1 = 0;
  filter->f4.z1 = 0;
}

static void filter_calculate_coefficients(struct frugi_filter *filter)
{
  float lfo_mod = filter->lfo_values[filter->lfo_function] * filter->lfo_depth;
  float env_mod = *filter->env_level * filter->env_depth;

  filter->actual_cutoff = filter->cutoff * MATH_POW(filter->lfo_range, lfo_mod) * MATH_EXP(env_mod);

  if (filter->actual_cutoff < 80)
  {
    filter->actual_cutoff = 80;
  }
  else if (filter->actual_cutoff > 18000)
  {
    filter->actual_cutoff = 18000;
  }

  float g = (TWO_OVER_T * tanf((DAE_TWO_PI * filter->actual_cutoff) * T_OVER_2)) * T_OVER_2;
  float G = g / (1.0f + g);

  filter->f1.alpha = G;
  filter->f2.alpha = G;
  filter->f3.alpha = G;
  filter->f4.alpha = G;

  filter->f1.beta = G * G * G / (1.0f + g);
  filter->f2.beta = G * G / (1.0f + g);
  filter->f3.beta = G / (1.0f + g);
  filter->f4.beta = 1.0 / (1.0f + g);

  float gamma = G * G * G * G;
  filter->alpha0 = 1.0f / (1.0f + filter->resonance * gamma);
}

typedef void (*output_func)(float *output, float U, float lpf1, float lpf2, float lpf3, float lpf4);
typedef void (*filter_func)(struct frugi_filter *, size_t, output_func select_taps);

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

void filter_render(struct frugi_filter * filter, size_t block_size, output_func select_taps)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(filter->sample_buf != NULL);
  RTT_ASSERT(filter->lfo_values != NULL);
  RTT_ASSERT(filter->env_level != NULL);
  RTT_ASSERT(select_taps != NULL);

  /* Stack cache variables */
  float alpha0 = filter->alpha0;
  float resonance = filter->resonance;
  float saturate = filter->saturation;

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

  float *restrict ptr = filter->sample_buf;
  float *restrict end = ptr + block_size;

  while (ptr < end)
  {
    float sigma = beta1 * z1 + beta2 * z2 + beta3 * z3 + beta4 * z4;
    float U = *ptr - resonance * sigma * alpha0;

    if (saturate)
    {
      U *= saturate;
      U = U / (FAST_FABS(2 * U) + 1.5f);
    }

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
    {filter_render, output_lpf2},
    {filter_render, output_bpf2},
    {filter_render, output_hpf2},
    {filter_render, output_lpf4},
    {filter_render, output_bpf4},
    {filter_render, output_hpf4}};

void frugi_filter_render(struct frugi_filter * filter, size_t block_size)
{
  RTT_ASSERT(filter != NULL);
  RTT_ASSERT(filter->sample_buf != NULL);
  RTT_ASSERT(filter->lfo_values != NULL);
  RTT_ASSERT(filter->env_level != NULL);

  filter_calculate_coefficients(filter);
  filter_funcs[filter->mode].filter(filter, block_size, filter_funcs[filter->mode].output);
}

void frugi_filter_update_params(struct frugi_filter *filter, float mode, float cutoff, float resonance,
                                float saturation, float lfo_depth, float lfo_function, float env_depth, float lfo_range, float note_tracking)
{
  RTT_ASSERT(filter != NULL);

  filter->mode = PARAM_TO_INT(mode, 0, FILTER_MODE_COUNT);
  filter->cutoff = PARAM_TO_EXP(cutoff, FILTER_CUT_MIN, FILTER_CUT_MAX);  
  filter->resonance = PARAM_TO_LINEAR(resonance, FILTER_RES_MIN, FILTER_RES_MAX);
  filter->saturation = PARAM_TO_LINEAR(saturation, FILTER_SAT_MIN, FILTER_SAT_MAX);
  filter->note_tracking = note_tracking;
  

  filter->lfo_depth = lfo_depth;
  filter->lfo_function = lfo_function;
  filter->lfo_range = lfo_range == LFO_RANGE_NORMAL ? FILTER_MOD_NORMAL : FILTER_MOD_EXTREME;
  filter->env_depth = env_depth;
}

void frugi_filter_note_on(struct frugi_filter *filter, uint8_t note)
{
  RTT_ASSERT(filter != NULL);

  if (filter->note_tracking)
  {
    
  }
}