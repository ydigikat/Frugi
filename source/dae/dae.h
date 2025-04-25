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
#ifndef DAE_H
#define DAE_H

#include <math.h>
#include <stdalign.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "midi.h"
#include "dae.h"
#include "params.h"
#include "dsp_core.h"

/* Intellisense has trouble with these in C23 for some reason, 
   this just gets rid of syntax error squiggles when intellisense parses */
#if defined(__INTELLISENSE__) || defined(__VSCODE_INTELLISENSE__)      
#define true (1)
#define false (!true)   
#endif

enum buffer_idx
{
  PING = 0,
  PONG = 1
};

/* API */
bool dae_start(UBaseType_t priority);
void dae_ready_for_audio(enum buffer_idx buffer_idx);
void dae_midi_received(uint8_t byte);

/* Synthesiser weak functions */
void dae_prepare_for_play(float sample_rate, size_t block_size, uint8_t *midi_channel);
void dae_update_parameters();
void dae_process_block(float *left, float *right, size_t block_size);
void dae_handle_midi(struct midi_msg *msg);

/* Audio subsystem weak function */
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock);

#endif /* DAE_H */