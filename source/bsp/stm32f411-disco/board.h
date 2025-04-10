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
#ifndef __PINS_H__
#define __PINS_H__

#include "core_board.h"

/* LEDs */
#define LED_GREEN_ON() (LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_12))
#define LED_ORANGE_ON() (LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_13))
#define LED_RED_ON() (LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_14))
#define LED_BLUE_ON() (LL_GPIO_SetOutputPin(GPIOD,LL_GPIO_PIN_15))

#define LED_GREEN_OFF() (LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_12))
#define LED_ORANGE_OFF() (LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_13))
#define LED_RED_OFF() (LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_14))
#define LED_BLUE_OFF() (LL_GPIO_ResetOutputPin(GPIOD,LL_GPIO_PIN_15))

#define USR_LED_ON() LED_GREEN_ON()
#define USR_LED_OFF() LED_GREEN_OFF()

/* USER Button */
#define READ_USR_BTN() (LL_GPIO_ReadInputPort(GPIOA) & LL_GPIO_PIN_0)

/* LOGIC probe */
#define D0_SET() (GPIOB->BSRR |= GPIO_BSRR_BS0)  
#define D1_SET() (GPIOB->BSRR |= GPIO_BSRR_BS1)  
#define D2_SET() (GPIOB->BSRR |= GPIO_BSRR_BS2)  
#define D3_SET() (GPIOE->BSRR |= GPIO_BSRR_BS8)  
#define D4_SET() (GPIOC->BSRR |= GPIO_BSRR_BS4)  
#define D5_SET() (GPIOC->BSRR |= GPIO_BSRR_BS5)  
#define D6_SET() (GPIOE->BSRR |= GPIO_BSRR_BS7)  
#define D7_SET() (GPIOE->BSRR |= GPIO_BSRR_BS9)  

/* SYS CLOCK */
#define PLL_M (LL_RCC_PLLM_DIV_4)
#define PLL_N (100)
#define PLL_R (LL_RCC_PLLP_DIV_2)
#define FREQ (100000000)

/* I2S */
#define I2S (SPI3)
#define I2S_AF (LL_GPIO_AF_6)
#define I2S_WS_PIN (LL_GPIO_PIN_4)
#define I2S_WS_PORT (GPIOA)
#define I2S_SDO_PIN (LL_GPIO_PIN_12)
#define I2S_SDO_PORT (GPIOC)
#define I2S_SCK_PIN (LL_GPIO_PIN_10)
#define I2S_SCK_PORT (GPIOC)
#define I2S_MCK_PIN (LL_GPIO_PIN_7)
#define I2S_MCK_PORT (GPIOC)


/* I2S PLL divider, we want divided down to 1MHz */
#define I2S_PLL_M (LL_RCC_PLLI2SM_DIV_8)    

/* DMA (I2S)*/
#define DMA (DMA1)
#define DMA_STREAM (LL_DMA_STREAM_5)
#define DMA_CHANNEL (LL_DMA_CHANNEL_0)
#define DMA_HISR_TCIF (DMA_HISR_TCIF5)
#define DMA_HIFCR_CTCIF (DMA_HIFCR_CTCIF5)
#define DMA_HIFCR_CHTIF (DMA_HIFCR_CHTIF5)

#define DMA_IRQN (DMA1_Stream5_IRQn)
#define DMA_IRQ_HANDLER DMA1_Stream5_IRQHandler

/* UART RX for MIDI */
#define UART (USART2)
#define UART_AF (LL_GPIO_AF_7)
#define UART_RX_PIN (LL_GPIO_PIN_3)
#define UART_RX_PORT (GPIOA)

#define UART_IRQN (USART2_IRQn)
#define UART_IRQ_HANDLER USART2_IRQHandler

#endif /* __BOARD_H__ */