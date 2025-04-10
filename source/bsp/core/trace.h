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
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#ifdef RTT_ENABLED

/* 
 * RTT (Real-Time Transfer) is like a supercharged serial port for debugging
 * It lets you see debug messages without slowing down your synth code
 * Much faster than traditional UART debugging!
 */
#include "SEGGER_RTT.h"

#ifdef DEBUG
/* 
 * Assertion macro - this is like a sanity check for your code
 * Only active when both RTT and DEBUG are enabled
 * 
 * Example: RTT_ASSERT(buffer != NULL);  // Make sure buffer exists before using it
 *
 * If the check fails, it prints the file, function and line number where it happened
 * then hangs the system (infinite loop) since continuing would be dangerous
 */
#define RTT_ASSERT(expr)                                                                                        \
  do                                                                                                            \
  {                                                                                                             \
    if (!(expr))                                                                                                \
    {                                                                                                           \
      SEGGER_RTT_printf(0, "ASSERTION FAILED: %s, function: %s, line: %d\n", __FILE__, __FUNCTION__, __LINE__); \
      while (1)                                                                                                 \
        ;                                                                                                       \
    }                                                                                                           \
  } while (0)
#else
/* In release builds, assertions do nothing (they're replaced with empty code) */
#define RTT_ASSERT(expr) ((void)0)
#endif /* DEBUG */

/* These require RTT and work for both DEBUG/RELEASE presets */

/* 
 * Use this when you need to print floating point values
 * Regular RTT_LOG can't handle floats directly
 * 
 * Example: RTT_LOG_FLOAT("Filter cutoff: %.2f Hz\n", cutoff);
 *
 * NOTE: This is slower because it uses snprintf to format the string first
 */
#define RTT_LOG_FLOAT(fmt, ...)               \
  do                                          \
  {                                           \
    char buffer[96];                          \
    snprintf(buffer, 96, fmt, ##__VA_ARGS__); \
    SEGGER_RTT_printf(0, "%s", buffer);       \
  } while (0)

/*
 * The main debug print function - fast and efficient
 * Use this for most debug messages when you don't need floating point
 *
 * Example: RTT_LOG("MIDI note received: %d\n", midi_note);
 */
#define RTT_LOG(fmt, ...) SEGGER_RTT_printf(0, fmt, ##__VA_ARGS__)

/*
 * DWT cycle counter setup - used for timing measurements
 * This initializes the ARM Cortex debug hardware for cycle counting
 * 
 * Put this at the beginning of a function where you want to measure performance
 */
#define DWT_INIT()                                \
  uint32_t dwt_start, dwt_end, dwt_cycles;        \
  uint32_t dwt_time_us;                           \
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; \
  DWT->CYCCNT = 0;                                \
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

/*
 * Resets and starts the cycle counter
 * Call this right before the code you want to measure
 */
#define DWT_CLEAR() \
  DWT->CYCCNT = 0;  \
  dwt_start = DWT->CYCCNT;

/*
 * Conversion factor from CPU cycles to microseconds
 * This assumes a 100MHz clock (each cycle is 10ns)
 * The fixed-point scaling gives better precision without floating point
 */
#define DWT_SCALE_FIXED ((1000000LL << 16) / 100000000) 

/*
 * Outputs the timing measurement
 * Call this right after the code you want to measure
 * Shows both CPU cycles and approximate time in microseconds
 *
 * Example:
 *   DWT_INIT();
 *   DWT_CLEAR();
 *   your_expensive_function();
 *   DWT_OUTPUT("Filter calculation");
 */
#define DWT_OUTPUT(msg)                               \
  dwt_end = DWT->CYCCNT;                              \
  dwt_cycles = dwt_end - dwt_start;                   \
  dwt_time_us = (dwt_cycles * DWT_SCALE_FIXED) >> 16; \
  RTT_LOG("%s # %s : %lu cycles (~%lu us)\n", RTT_CTRL_TEXT_BRIGHT_CYAN, msg, dwt_cycles, dwt_time_us);

#else
/*
 * When RTT is disabled, all debug macros become empty
 * This ensures debug code doesn't affect your synth when building for performance
 * Your synth code stays clean without having to remove debug statements
 */
#define RTT_LOG(...)
#define RTT_LOG_FLOAT(fmt, ...)
#define RTT_ASSERT(expr)
#define DWT_INIT()
#define DWT_CLEAR()
#define DWT_OUTPUT(function)

#endif /* RTT_ENABLED */

#endif /* __DEBUG_H__ */