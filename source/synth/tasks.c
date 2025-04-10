/*
  ------------------------------------------------------------------------------
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission to use, copy, modify, and/or distribute this code for any purpose
   with or without fee is hereby granted, provided the above copyright notice and
   this permission notice appear in all copies.
  ------------------------------------------------------------------------------
*/
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "dae.h"
#include "ui.h"

#include "frugi_synth.h"

extern bool dae_start(UBaseType_t priority);

/* The one and only synth instance */
struct frugi_synth frugi_synth;


static void handle_error(void)
{
  while (1)
    ;
}

void start_tasks(void *args)
{
  if (!dae_start(tskIDLE_PRIORITY + 3))
  {
    RTT_LOG("DAE task failed to start\n");
    handle_error();
  }

  if (!ui_start(tskIDLE_PRIORITY + 1))
  {
    RTT_LOG("UI task failed to start\n");
    handle_error();
  }

  /* Not needed now that the other tasks are running */
  vTaskDelete(xTaskGetCurrentTaskHandle());
}


void dae_prepare_for_play(float sample_rate, size_t block_size, uint8_t *midi_channel)
{
  frugi_synth_init(&frugi_synth, sample_rate, block_size, midi_channel);
}

void dae_update_parameters()
{
  frugi_synth_update_params(&frugi_synth);
}

void dae_process_block(float *left, float *right, size_t block_size)
{
  // DWT_INIT();
  // DWT_CLEAR();
  frugi_synth_render(&frugi_synth, left, right, block_size);
  // DWT_OUTPUT("sc_dae_process_block");
}

void dae_handle_midi(struct midi_msg *msg)
{
  frugi_synth_midi_message(&frugi_synth, msg->data[0], msg->data[1], msg->data[2]);
}
