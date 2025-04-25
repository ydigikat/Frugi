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

#include "board.h"

/* On-board CODEC */
extern void cs43l22_init(void);

/**
 * enable_peripheral_clocks
 * \brief Enable the specific clocks needed by this board
 * \note core_board_init() will call this during initialisation of peripherals.
 */
void enable_peripheral_clocks()
{
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOE);

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);  
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
}

/**
 * gpio_init
 * \brief Enable board specific LEDs, BTNS etc
 * \note core_board_init() will call this during initialisation of peripherals.
 */
bool gpio_init()
{
  /* LEDs */
  LL_GPIO_InitTypeDef led =
      {
          .Mode = LL_GPIO_MODE_OUTPUT,
          .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
          .Pull = LL_GPIO_PULL_DOWN,
          .Pin = LL_GPIO_PIN_12 | LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15};

  if (LL_GPIO_Init(GPIOD, &led) != SUCCESS)
  {
    return false;
  }

  /* USR button */
  LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_0, LL_GPIO_PULL_UP);
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);

  /* Probe toggles */
  LL_GPIO_InitTypeDef probe =
      {
          .Mode = LL_GPIO_MODE_OUTPUT,
          .Speed = LL_GPIO_SPEED_FREQ_HIGH,
          .Pull = LL_GPIO_PULL_DOWN,
          .Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_8 | LL_GPIO_PIN_7 | LL_GPIO_PIN_6 | LL_GPIO_PIN_5 | LL_GPIO_PIN_4 |
                 LL_GPIO_PIN_3 | LL_GPIO_PIN_2};

  if (LL_GPIO_Init(GPIOB, &probe) != SUCCESS)
  {
    return false;
  }

  return true;
}

/**
 * board_init
 * \brief initialise the board hardware
 * \note core_board_init() does most of the heavy lifting here.  Any additional
 *       board-specific peripherals should be initialised here.
 */
bool board_init()
{
  if(!core_board_init())
  {
    return false;
  }

  /* Driver for onboard DAC */
  cs43l22_init();

  return true;
}

