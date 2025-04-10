/*
  ------------------------------------------------------------------------------
  SynthCoreF4
  Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission to use, copy, modify, and/or distribute this code for any purpose
   with or without fee is hereby granted, provided the above copyright notice and
   this permission notice appear in all copies.
  ------------------------------------------------------------------------------
*/
#include "FreeRTOS.h"
#include "task.h"
#include "board.h"
#include "trace.h"

extern void start_tasks(void *args);

int main(void)
{
  board_init();  
   
  if(xTaskCreate(start_tasks, "start_tasks", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL)== pdPASS)
  {
      vTaskStartScheduler();
  }

  /* We shouldn't get here */
  RTT_LOG("%sFailed to start scheduler\n",RTT_CTRL_TEXT_BRIGHT_RED);  
  while (1);
  
  /* NOTREACHED*/
  return 0;
}

/* This will be called if RTOS detects that we're stomping over the stack */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{ 
    RTT_LOG("%sStack overflow, task:%s\n",RTT_CTRL_TEXT_BRIGHT_RED,pcTaskName);       
    while(1);
}

/* This will be called if any dynamic allocation of memory using pvPortAlloc fails*/
void vApplicationMallocFailedHook(void)
{
    RTT_LOG("%sApplication malloc() failed.",RTT_CTRL_TEXT_BRIGHT_RED);       
    while(1);
}