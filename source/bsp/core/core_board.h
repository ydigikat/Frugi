/*
  ------------------------------------------------------------------------------
   BSP-F4
   Author: ydigikat
  ------------------------------------------------------------------------------
   MIT License
   Copyright (c) 2025 YDigiKat

   Permission to use, copy, modify, and/or distribute this code for any purpose
   with or without fee is hereby granted, provided the above copyright notice and
   this permission notice appear in all copies.
  ------------------------------------------------------------------------------
*/
#ifndef __CORE_BOARD_H__
#define __CORE_BOARD_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "stm32f4xx.h"

#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_spi.h"
#include "stm32f4xx_ll_dma.h"
#include "stm32f4xx_ll_usart.h"

#include "trace.h"
#include "board.h"

/* Optional full assert */
#if defined(USE_FULL_ASSERT)
#include "stm32_assert.h"
#endif 


/* Intellisense has trouble with these in C23 for some reason, 
   this just gets rid of syntax error squiggles when intellisense parses */
#if defined(__INTELLISENSE__) || defined(__VSCODE_INTELLISENSE__)      
#define true 
#define false     
#endif

/* I2S PLL dividers are the same for all boards providing the oscillator is divided down
   to 1MHz or 2MHz, these come from Table 90 in the STM32F411xE reference manual */
#ifdef DAE_IS_USING_MCLOCK

#define I2S_44_N  (271)
#define I2S_44_R  (LL_RCC_PLLI2SR_DIV_2)

#define I2S_48_N  (258)
#define I2S_48_R  (LL_RCC_PLLI2SR_DIV_3)

#define I2S_96_N  (344)
#define I2S_96_R  (LL_RCC_PLLI2SR_DIV_2)

#else

#define I2S_44_N  (429)
#define I2S_44_R  (LL_RCC_PLLI2SR_DIV_4)

#define I2S_48_N  (384)
#define I2S_48_R  (LL_RCC_PLLI2SR_DIV_5)

#define I2S_96_N  (424)
#define I2S_96_R  (LL_RCC_PLLI2SR_DIV_3)

#endif


/* Board setup */
void enable_peripheral_clocks();
bool gpio_init();
bool core_board_init(void);

/* API */
bool board_init(void);




#endif /* __CORE_BOARD_H__ */