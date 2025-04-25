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
#ifndef __BOARD_H__
#define __BOARD_H__

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
#include "core_board.h"

/* Set LED state */
#define USR_LED_OFF() (LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_13))
#define USR_LED_ON() (LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_13))

/* Read Button, this is on bit 1 of the port */
#define READ_USR_BTN() (LL_GPIO_ReadInputPort(GPIOA) & LL_GPIO_PIN_0)

/* Logic check pins */
#define D0_SET() (GPIOB->BSRR |= GPIO_BSRR_BS9)  
#define D0_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR9) 
#define D1_SET() (GPIOB->BSRR |= GPIO_BSRR_BS8)  
#define D1_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR8) 
#define D2_SET() (GPIOB->BSRR |= GPIO_BSRR_BS7)  
#define D2_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR7) 
#define D3_SET() (GPIOB->BSRR |= GPIO_BSRR_BS6)  
#define D3_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR6) 
#define D4_SET() (GPIOB->BSRR |= GPIO_BSRR_BS5)  
#define D4_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR5) 
#define D5_SET() (GPIOB->BSRR |= GPIO_BSRR_BS4)  
#define D5_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR4) 
#define D6_SET() (GPIOB->BSRR |= GPIO_BSRR_BS3)  
#define D6_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR3) 
#define D7_SET() (GPIOB->BSRR |= GPIO_BSRR_BS2)  
#define D7_CLEAR() (GPIOB->BSRR |= GPIO_BSRR_BR2) 

/* SYS CLOCK */
#define PLL_M (LL_RCC_PLLM_DIV_12)
#define PLL_N (96)
#define PLL_R (LL_RCC_PLLP_DIV_2)
#define FREQ (100000000)

/* I2S */
#define I2S (SPI2)
#define I2S_AF (LL_GPIO_AF_5)
#define I2S_WS_PIN (LL_GPIO_PIN_12)
#define I2S_WS_PORT (GPIOB)
#define I2S_SDO_PIN (LL_GPIO_PIN_15)
#define I2S_SDO_PORT (GPIOB)
#define I2S_SCK_PIN (LL_GPIO_PIN_10)
#define I2S_SCK_PORT (GPIOB)
#define I2S_MCK_PIN (LL_GPIO_PIN_3)
#define I2S_MCK_PORT (GPIOA)

/* I2S PLL divider, we want divided down to 1MHz */
#define I2S_PLL_M (LL_RCC_PLLI2SM_DIV_25)  

/* DMA (I2S)*/
#define DMA (DMA1)
#define DMA_IRQN (DMA1_Stream4_IRQn)
#define DMA_STREAM (LL_DMA_STREAM_4)
#define DMA_CHANNEL (LL_DMA_CHANNEL_0)
#define DMA_HISR_TCIF (DMA_HISR_TCIF4)
#define DMA_HIFCR_CTCIF (DMA_HIFCR_CTCIF4)
#define DMA_HIFCR_CHTIF (DMA_HIFCR_CHTIF4)

#define DMA_IRQN (DMA1_Stream4_IRQn)
#define DMA_IRQ_HANDLER DMA1_Stream4_IRQHandler

/* UART RX for MIDI */
#define UART (USART1)
#define UART_AF (LL_GPIO_AF_7)
#define UART_RX_PIN (LL_GPIO_PIN_10)
#define UART_RX_PORT (GPIOA)

#define UART_IRQN (USART1_IRQn)
#define UART_IRQ_HANDLER USART1_IRQHandler


#endif /* __BOARD_H__ */