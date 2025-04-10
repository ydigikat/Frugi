#ifndef __FILTER_H__
#define __FILTER_H__

#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>
#include <math.h>


#include "trace.h"



#include "frugi_params.h"
#include "dsp_core.h"
#include "dsp_math.h"

enum filter_tap_col
{
  A_TAP = 0,
  B_TAP,
  C_TAP,
  D_TAP,
  E_TAP
};

struct sub_filter
{
  float alpha;
  float beta;
  float a0;
  float z1;
};

struct frugi_filter
{

  /* I/O */
  float *sample_buf;
  float *lfo_values;
  float *env_level;

  /* User parameters */
  enum frugi_filter_mode mode;
  float cutoff;
  float resonance;
  float saturation;

  enum frugi_lfo_function lfo_function;
  float lfo_depth;
  float env_depth;
  float lfo_range;

  bool note_tracking;
  uint8_t note;

  /* Private data */
  float fsr;
  float actual_cutoff;
  float actual_resonance;
  float bass_comp;
  float alpha0;

  /* Filter cascade */
  struct sub_filter f1, f2, f3, f4;
};

void frugi_filter_init(struct frugi_filter *filter, float fsr, float *sample_buf, float *lfo_values, float *env_level);
void frugi_filter_reset(struct frugi_filter *filter);
void frugi_filter_note_on(struct frugi_filter *filter, uint8_t note);
void frugi_filter_render(struct frugi_filter *filter, size_t block_size);
void frugi_filter_update_params(struct frugi_filter *filter, float mode, float cutoff,
                                float resonance, float saturation, float depth, float function,
                                float env_depth, float lfo_range, float note_tracking);

#endif /* __FILTER_H__ */