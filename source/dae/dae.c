/*
  ------------------------------------------------------------------------------
   DAE
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
  ------------------------------------------------------------------------------
*/
#include "dae.h"


/* Configuration */
#ifndef DAE_IS_USING_MCLOCK
#define DAE_IS_USING_MCLOCK (0)
#endif

#ifndef DAE_SAMPLE_RATE
#define DAE_SAMPLE_RATE (48000)
#endif

#ifndef DAE_AUDIO_BLOCK_SIZE
#define DAE_AUDIO_BLOCK_SIZE (128)
#endif

#define DAE_AUDIO_BUFFER_SIZE (DAE_AUDIO_BLOCK_SIZE * 8)

/* Coefficients for test tone generator */
static const float test_tone_b_coeff = 1.27323954474f;
static const float test_tone_c_coeff = -0.40528473456f;
static const float test_tone_p_coeff = 0.225f;
static float test_tone_phase = 0;
static float test_tone_inc = 440.0f / DAE_SAMPLE_RATE;

/* Sample and audio buffers */
static __attribute__((aligned(4))) float left_buffer[DAE_AUDIO_BLOCK_SIZE];
static __attribute__((aligned(4))) float right_buffer[DAE_AUDIO_BLOCK_SIZE];
static __attribute__((aligned(2))) int16_t audio_buffer[DAE_AUDIO_BUFFER_SIZE];

/* Externals */
extern bool dae_param_changed;
extern void dae_param_init(void);

/* State variables */
static uint8_t active_buffer = PONG;
static TaskHandle_t dae_task_handle;
static struct midi_port midi_in;

/* Forward declarations of private functions */
static void check_buffer(float *buffer, int sampleCount);
static void generate_test_tone(float *restrict left, float *restrict right, size_t block_size);
static float get_actual_fsr(uint32_t fsr_selected, bool mclk_enabled);

/**
 * dae_task
 * \brief This is the main audio processing thread (task)
 * \param pvParameters - unused
 */
static void dae_task(void *pvParameters)
{
  /* Starts the board audio subsystem (I2S and DMA peripherals) */
  audio_start(audio_buffer, DAE_AUDIO_BUFFER_SIZE, DAE_SAMPLE_RATE, DAE_IS_USING_MCLOCK);  

  /* Initialises the global parameter store */
  dae_param_init();

  /* Configures the sound source for playing, passing it DAE parameters and obtaining the MIDI channel */
  dae_prepare_for_play(get_actual_fsr(DAE_SAMPLE_RATE, DAE_IS_USING_MCLOCK), DAE_AUDIO_BLOCK_SIZE, &midi_in.channel);

  while (1)
  {
    /* Sleep until the DMA signals us to refresh a buffer */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    DWT_INIT();
    DWT_CLEAR();

    /* Pass the synthesiser any buffered MIDI at the start of the block, this is block level and
       messages are not time-stamped so MIDI slicing is not possible. */
    uint8_t byte;
    while (midi_buffer_read(&byte))
    {
      struct midi_msg *msg = midi_parse(&midi_in, byte);
      if (msg != NULL)
      {
        dae_handle_midi(msg);
      }
    }

    /* Inform the synthesiser if it needs to refresh any changed parameters */
    if (dae_param_changed)
    {
      dae_update_parameters();
      dae_param_changed = false;
    }

    /* Call synthesiser to generate the audio block */
    dae_process_block(left_buffer, right_buffer, DAE_AUDIO_BLOCK_SIZE);

    /* Select the buffer to which audio is output */
    int16_t *restrict ptr = (active_buffer == PING) ? audio_buffer : audio_buffer + DAE_AUDIO_BUFFER_SIZE / 2;

#ifdef DAE_CHECK_BUFFER
    /* Sanitises  buffers to protect speakers (ears) in debug mode */
    check_buffer(left_buffer, DAE_AUDIO_BLOCK_SIZE);
    check_buffer(left_buffer, DAE_AUDIO_BLOCK_SIZE);
#endif

/* Copy samples to audio buffer in I2S required format */
#pragma GCC unroll 4
    for (int i = 0; i < DAE_AUDIO_BLOCK_SIZE; i++)
    {
      int32_t l_sample = left_buffer[i] * INT32_MAX;
      int32_t r_sample = right_buffer[i] * INT32_MAX;

      *ptr++ = (int16_t)(r_sample >> 16);
      *ptr++ = (int16_t)(r_sample);
      *ptr++ = (int16_t)(l_sample >> 16);
      *ptr++ = (int16_t)(l_sample);
    }

    DWT_OUTPUT("DAE");
  }
}

/**
 * dae_start
 * \brief This kicks off the DAE thread (RTOS task)
 * \param priority The FreeRTOS task priority.
 */
bool dae_start(UBaseType_t priority)
{
  if (xTaskCreate(dae_task, "DAE", configMINIMAL_STACK_SIZE * 4, NULL, priority, &dae_task_handle) != pdPASS)
  {
    return false;
  }

  return true;
}


/**
 * dae_ready_for_audio
 * \brief called by the audio hardware when a new buffer of audio sample is required.
 * \param buffer_idx the buffer to refill (PING or PONG)
 */
void dae_ready_for_audio(enum buffer_idx buffer_idx)
{
  BaseType_t higher_task_woken = pdFALSE;

  active_buffer = buffer_idx;

  /* Notify the DAE task that it is ready to process audio */
  vTaskNotifyGiveFromISR(dae_task_handle, &higher_task_woken);

  /* If the DAE task has higher priority than the current interrupted task, yield */
  if (higher_task_woken)
  {
    portYIELD_FROM_ISR(higher_task_woken);
  }
}

/**
 * dae_midi_received
 * \brief called by the hardware when a MIDI byte is received.  The DAE adds these to the
 *        MIDI ring_buffer for processing at the start of the next audio block.
 * \param byte the data byte.
 */
void dae_midi_received(uint8_t byte)
{
  if (byte == MIDI_STATUS_ACTIVE_SENSE)
  {
    return;
  }
  /* Write the byte to the MIDI ring-buffer */
  midi_buffer_write(byte);  
}


/**
 * dae_perpare_to_play 
 * \brief called by the DAE when it is starting the audio task.  
 * \param sample_rate the sample rate
 * \param block_size the audio block size for dynamic buffer allocation etc.
 * \param midi_channel the midi channel wanted, the synthesiser should set this value.
 */
__attribute__((weak)) void dae_prepare_for_play(float sample_rate, size_t block_size, uint8_t *midi_channel)
{
  /* Override this in your synthesiser */
}


/**
 * dae_update_parameters()
 * \brief called by the DAE when it detects that one or more parameters in its store have changed.
 */
__attribute__((weak)) void dae_update_parameters()
{
  /* Override this in your synthesiser, use the param_get* function calls to retrieve parameter values */
}


/**
 * dae_process_block()
 * \brief called by the DAE when it requires a new block of samples
 * \param left the left sample buffer
 * \param right the right sample buffer
 * \param block_size the number of samples required.
 */
__attribute__((weak)) void dae_process_block(float *left, float *right, size_t block_size)
{
  /* Override this in your synthesiser, the base call will generate a 440Hz continuout sine tone */
  generate_test_tone(left, right, block_size);
}


/**
 * dae_handle_midi()
 * \brief called by DAE to pass MIDI data to then synthesiser, this can be called multiple times at the
 *        start of a block.
 * \param msg a valid parsed MIDI message.
 */
__attribute__((weak)) void dae_handle_midi(struct midi_msg *msg)
{
  /* Override this to handle MIDI messages in your synthesiser*/
}

/**
 * audio_start()
 * \brief called by the DAE to initialise audio hardware.
 * \param audio_buffer the audio sample buffer
 * \param buf_len the size of the sample buffer
 * \param fsr Sample rate
 */
__attribute__((weak)) void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock)
{
  /* Override this in your hardware layer to start audio and store the shared buffer */
}


/**
 * generate_test_tone();
 * \brief This generates a sine approximation at 440Hz for testing.
 * \param left buffer
 * \param right buffer
 * \param block_size size of buffer
 */
static void generate_test_tone(float *restrict left, float *restrict right, size_t block_size)
{
  RTT_ASSERT(left != NULL);
  RTT_ASSERT(right != NULL);
  RTT_ASSERT(block_size > 0);

  for (size_t i = 0; i < block_size; i++)
  {
    if (test_tone_phase > 1.0f)
    {
      test_tone_phase -= 1.0f;
    }

    float angle = -1.0f * (test_tone_phase * 2.0f * DAE_PI - DAE_PI);
    float y = test_tone_b_coeff * angle + test_tone_c_coeff * angle * fabsf(angle);

    left[i] = (test_tone_p_coeff * (y * fabsf(y) - y) + y);
    right[i] = left[i];
    test_tone_phase += test_tone_inc;
  }
}

/**
 * get_actual_fsr
 * \brief Returns the actual sample rate, the PLL in the STM cannot
 *        generate the exact sample rates, so this function returns the
 *        rate reported by the STM.  This can then be compensated for
 *        in the synthesiser.
 * \param fsr_selected - the selected sample rate
 * \param mclk_enabled - true if the MCLK is enabled
 * \return the actual sample rate
 */
static float get_actual_fsr(uint32_t fsr_selected, bool mclk_enabled)
{
  // if (fsr_selected == 44100)
  // {
  //   return mclk_enabled ? 45072 : 44221;
  // }
  // if (fsr_selected == 48000)
  // {
  //   return mclk_enabled ? 48828 : 47831;
  // }
  // if (fsr_selected == 96000)
  // {
  //   return mclk_enabled ? 97656 : 97656;
  // }
  return fsr_selected;
}

#include <string.h>

#ifdef DAE_CHECK_BUFFER

/**
 * check_buffer
 * \brief Checks the buffer for NaN, infinity and clipping
 * \param buffer - the buffer
 * \param sampleCount - the sample count
 */
static void check_buffer(float *buffer, int sampleCount)
{
  if (!buffer)
    return;

  for (int i = 0; i < sampleCount; ++i)
  {
    const float x = buffer[i];

    if (isnan(x) || isinf(x) || fabs(x) > 2.0f)
    {
      memset(buffer, 0, sampleCount * sizeof(float));
      return;
    }

    if (fabs(x) > 1.0f)
    {
      buffer[i] = x > 0 ? 1.0f : -1.0f;
    }
  }
}

#endif