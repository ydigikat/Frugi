#ifndef __NOISE_H
#define __NOISE_H

#include <stdint.h>

#include "trace.h"



#include "frugi_params.h"
#include "dsp_core.h"



struct frugi_noise
{
  /* I/O */
  float *sample_buf;

  /* Parameters */
  float level;
  enum frugi_noise_type type;
  
  /* Private Data */
  bool note_on;
};

void frugi_noise_init(struct frugi_noise *noise, float fsr, float *io_buffer);
void frugi_noise_reset(struct frugi_noise *noise);
void frugi_noise_render(struct frugi_noise *noise, size_t block_size);
void frugi_noise_note_on(struct frugi_noise *noise);
void frugi_noise_note_off(struct frugi_noise *noise);
void frugi_noise_update_params(struct frugi_noise *noise, float level, float type);


#endif // __NOISE_H