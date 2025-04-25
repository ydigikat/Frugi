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
#include "core_board.h"


extern void dae_ready_for_audio(uint8_t buffer_idx); 
extern void dae_midi_received(uint8_t byte);         


 /**
  * \brief This configures the STM32F4's clock system.
  * \note  - Enables the high-speed external clock source (HSE)
  *        - Sets up the PLL for the fastest possible CPU frequency
  *        - Configures he bus clock dividers
  */
static void clock_init()
{
  LL_RCC_HSE_Enable(); 
  while (!LL_RCC_HSE_IsReady())
    ;

  /* Power management settings */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);         
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1); 
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);                   

  /* Configure the PLL */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, PLL_M, PLL_N, PLL_R);
  LL_RCC_PLL_Enable(); 
  while (!LL_RCC_PLL_IsReady())
    ;

  /* Scale the system clock signal */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1); 
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL); 
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }; 

  /* Scale the bus clock signals - APB1 runs at half speed */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2); 
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1); 

  /* Tell the system our speed */
  SystemCoreClock = FREQ;                     
}

/**
 * Enables the Floating Point Unit (FPU)
 *
 * Audio DSP needs floating point math, and this chip has hardware acceleration for it.
 * This function turns it on so we can do audio calculations way faster!
 */

/**
 * \brief Enable the floating point unit (FPU)
 */
static void fpu_init()
{
  /* Enable */
  SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));  

  /* Subnormals as zeroes */
  __set_FPSCR(__get_FPSCR() | (1 << 24));
  __ISB(); 
  __DSB(); 
}

/**
 * \brief Configure the UART for MIDI input
 * \note  MIDI runs at 31250 baud, 8N1, RX only since the synth does not
 *        ever send MIDI.
 * 1\return true if successful, false if initialisation failed.
 */
static bool uart_init()
{
  
  LL_GPIO_InitTypeDef gpio =
      {
          .Mode = LL_GPIO_MODE_ALTERNATE,   
          .Alternate = UART_AF,             
          .Pin = UART_RX_PIN,               
          .Speed = LL_GPIO_SPEED_FREQ_HIGH, 
          .Pull = LL_GPIO_PULL_UP};         

  if (LL_GPIO_Init(UART_RX_PORT, &gpio) != SUCCESS)
  {
    return false; 
  }

  /* USART config  31250, 8N1 RX only */
  LL_USART_InitTypeDef usart =
      {
          .BaudRate = 31250,                           
          .OverSampling = LL_USART_OVERSAMPLING_16,    
          .DataWidth = LL_USART_DATAWIDTH_8B,          
          .Parity = LL_USART_PARITY_NONE,              
          .StopBits = LL_USART_STOPBITS_1,             
          .TransferDirection = LL_USART_DIRECTION_RX}; 

  if (LL_USART_Init(UART, &usart) != SUCCESS)
  {
    return false; 
  }

  /* Interrupt handling, this will signal arrival of MIDI data */
  NVIC_EnableIRQ(UART_IRQN);      
  NVIC_SetPriority(UART_IRQN, 6); 

  LL_USART_EnableIT_RXNE(UART); 

  /* Enable */
  LL_USART_Enable(UART); 
  while (!LL_USART_IsEnabled(UART))
    ; 

  return true;
}

/**
 * \brief Sets up the I2S interface for audio output
 * \note Philips standard, 32-bit on 32-bit frame.
 * \return true if successful, false if initialisation failed.
 */
static bool i2s_init()
{
  
  LL_GPIO_InitTypeDef io =
      {
          .Speed = LL_GPIO_SPEED_FREQ_HIGH,
          .Pull = LL_GPIO_PULL_DOWN,       
          .Mode = LL_GPIO_MODE_ALTERNATE,   
          .Alternate = I2S_AF,              
      };

  /* Master clock required for some DACs */
  io.Pin = I2S_MCK_PIN;
  if (LL_GPIO_Init(I2S_MCK_PORT, &io) != SUCCESS)
  {
    return false;
  }

  /* Serial Clock (SCK, aka BCLK) - the bit clock */
  io.Pin = I2S_SCK_PIN;
  if (LL_GPIO_Init(I2S_SCK_PORT, &io) != SUCCESS)
  {
    return false;
  }

  /* Serial Data Out (SDO, aka SD) - the audio data */
  io.Pin = I2S_SDO_PIN;
  if (LL_GPIO_Init(I2S_SDO_PORT, &io) != SUCCESS)
  {
    return false;
  }

  /* Word Select (WS, aka LRCK) - indicates left/right channel */
  io.Pin = I2S_WS_PIN;
  if (LL_GPIO_Init(I2S_WS_PORT, &io) != SUCCESS)
  {
    return false;
  }

  /* Configure I2S peripheral (initial - final configuration in audio_start) */
  LL_I2S_InitTypeDef i2s =
      {
          .AudioFreq = LL_I2S_AUDIOFREQ_44K,        
          .ClockPolarity = LL_I2S_POLARITY_LOW,     
          .DataFormat = LL_I2S_DATAFORMAT_32B,      
          .MCLKOutput = LL_I2S_MCLK_OUTPUT_DISABLE, 
          .Mode = LL_I2S_MODE_MASTER_TX,            
          .Standard = LL_I2S_STANDARD_PHILIPS};     

  if (LL_I2S_Init(I2S, &i2s) != SUCCESS)
  {
    return false;
  }

  /* I2S is will be started by audio_start() */
  return true;
}

/**
 * \brief set up the DMA for I2S audio output
 * \note DMA (Direct Memory Access) lets us transfer audio data without CPU involvement.
 * \return true if successful, false if initialisation failed.
 */
static bool dma_i2s_init()
{
  LL_DMA_InitTypeDef dma_i2s =
      {
          .Channel = DMA_CHANNEL,                               
          .Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,       
          .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,   
          .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,     
          .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD, 
          .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD, 
          .Mode = LL_DMA_MODE_CIRCULAR,                         
          .Priority = LL_DMA_PRIORITY_HIGH,                     
          .FIFOMode = LL_DMA_FIFOMODE_DISABLE};                 

  if (LL_DMA_Init(DMA, DMA_STREAM, &dma_i2s) != SUCCESS)
  {
    return false;
  }

  /* Enable interrupts at half-transfer (HT) and transfer-complete (TC) */
  LL_DMA_EnableIT_HT(DMA, DMA_STREAM); 
  LL_DMA_EnableIT_TC(DMA, DMA_STREAM); 

  NVIC_SetPriority(DMA_IRQN, 10);
  NVIC_EnableIRQ(DMA_IRQN);      

  /* Connnect DMA to I2S peripheral */
  LL_SPI_EnableDMAReq_TX(I2S);

  /* DMA is started in audio_start() */

  return true;
}

/**
 * \brief Main board initialization function
 * \note This sets up all the hardware:
 *        - System clocks
 *        - Floating point unit
 *        - Flash cache for faster code execution
 *        - GPIO pins
 *        - I2S audio output
 *        - DMA for buffer transfers
 *        - UART for MIDI input
 * \return true if successful, false if initialisation failed.
 */
bool core_board_init(void)
{
  /* Initialise system clock tree and FPU*/
  clock_init(); 
  fpu_init();   

  /* Enable ART flash cache subsystem */
  LL_FLASH_EnablePrefetch();  
  LL_FLASH_EnableInstCache(); 
  /* LL_FLASH_EnableDataCache();  */

  /* Turn on the clocks we'll be using */
  enable_peripheral_clocks(); 

  /* Initialise all the peripherals */
  if (!gpio_init())
  {
    RTT_LOG("%s gpio_init() failed.", RTT_CTRL_TEXT_RED);
    return false;
  }

  if (!i2s_init())
  {
    RTT_LOG("%s i2s_init() failed.", RTT_CTRL_TEXT_RED);
    return false;
  }

  if (!dma_i2s_init())
  {
    RTT_LOG("%s dma_i2s_init() failed.", RTT_CTRL_TEXT_RED);
    return false;
  }

  if (!uart_init())
  {
    RTT_LOG("%s uart_init() failed.", RTT_CTRL_TEXT_RED);
    return false;
  }

  return true; 
}

/**
 * audio_start
 * \brief called by the DAE to start the audio hardware (callback)
 * \note only set mclock true if you are using a master clock, otherwise the WS/BCLK rates will calculate incorrectly.
 * 
 * \param audio_buffer the audio buffer, this is split by the DAE into ping/pong halves.
 * \param buf_len the length of the audio buffer (32 bit words, interleaved stereo L+R)
 * \param fsr the sample rate (44100, 48000, 96000)
 * \param mclock true if you need a master clock (see note)
 */
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock)
{
  /* Set DMA transfer buffer */
  LL_DMA_SetDataLength(DMA, DMA_STREAM, buf_len); 
  LL_DMA_ConfigAddresses(DMA, DMA_STREAM, (uint32_t)audio_buffer, LL_SPI_DMA_GetRegAddr(I2S), LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  /* Calculate the PLL speeds required for different sample rates */
  if (fsr == 48000)
  {
    LL_RCC_PLLI2S_ConfigDomain_I2S(LL_RCC_PLLSOURCE_HSE, I2S_PLL_M, I2S_48_N, I2S_48_R);
  }
  else if (fsr == 96000)
  {
    LL_RCC_PLLI2S_ConfigDomain_I2S(LL_RCC_PLLSOURCE_HSE, I2S_PLL_M, I2S_96_N, I2S_96_R);
  }
  else
  {    
    LL_RCC_PLLI2S_ConfigDomain_I2S(LL_RCC_PLLSOURCE_HSE, I2S_PLL_M, I2S_44_N, I2S_44_R);
  }

  /* Re-enable PLLI2S */
  LL_RCC_PLLI2S_Enable(); 
  while (!LL_RCC_PLLI2S_IsReady())
    ; 

  /* Update I2S peripheral with new frequency */
  LL_I2S_InitTypeDef i2s =
      {
          .AudioFreq = fsr, 
          .ClockPolarity = LL_I2S_POLARITY_LOW,
          .DataFormat = LL_I2S_DATAFORMAT_32B,                                           
          .MCLKOutput = mclock ? LL_I2S_MCLK_OUTPUT_ENABLE : LL_I2S_MCLK_OUTPUT_DISABLE, 
          .Mode = LL_I2S_MODE_MASTER_TX,
          .Standard = LL_I2S_STANDARD_PHILIPS};
      
  if (LL_I2S_Init(I2S, &i2s) != SUCCESS)
  {
    return; 
  }

  /* Enable I2S */
  LL_I2S_Enable(I2S); 
  while (!LL_I2S_IsEnabled(I2S))
    ; 

  /* Enable DMA */
  LL_DMA_EnableStream(DMA, DMA_STREAM); 
  while (!LL_DMA_IsEnabledStream(DMA, DMA_STREAM))
    ; 

  /* Enable UART */
  LL_USART_Enable(UART); 
  while (!LL_USART_IsEnabled(UART))
    ; 
}

/**
 * DMA Interrupt Handler
 *
 * This gets called when the DMA has finished transferring half or all of the audio buffer.
 * It lets the Digital Audio Engine know that it's time to fill the next half of the buffer
 * with fresh audio data (ping-pong buffering technique).
 */
void DMA_IRQ_HANDLER(void)
{
  if (DMA->HISR & DMA_HISR_TCIF)
  {
    DMA->HIFCR = DMA_HIFCR_CTCIF; // Clear the transfer-complete flag
    dae_ready_for_audio(1);       // Tell DAE to fill the second half of the buffer
  }
  else
  {
    DMA->HIFCR = DMA_HIFCR_CHTIF; // Clear the half-transfer flag
    dae_ready_for_audio(0);       // Tell DAE to fill the first half of the buffer
  }
}

/**
 * UART Interrupt Handler
 *
 * This gets called whenever a MIDI byte arrives at the UART.
 * It simply passes the data to the Digital Audio Engine for processing.
 */
void UART_IRQ_HANDLER(void)
{
  
  dae_midi_received((uint8_t)UART->DR); // Send the received MIDI byte to the DAE
}



/**
 * \brief Hard fault exception handler
 * \note If the processor hits a serious error, this gets called.
 *       It logs the error and hangs the system (better than random behavior).
 */
void HardFault_Handler(void)
{
  RTT_LOG("HardFault_Handler\n");
  while (1)
    ; 
}

/**
 * \brief Memory management fault handler
 * \note Called on memory protection errors.
 *       Generally means trying to access memory it shouldn't.
 */
void MemManage_Handler(void)
{
  RTT_LOG("MemManage_Handler\n");
  while (1)
    ; 
}

/**
 * \brief Bus fault exception handler
 * \note Called on errors during memory transfers.
 *       Typically means trying to access hardware that isn't there.
 */
void BusFault_Handler(void)
{
  RTT_LOG("BusFault_Handler\n");
  while (1)
    ; 
}

/**
 * \brief Usage fault exception handler
 * \note Called on various CPU usage errors like trying to execute invalid instructions.
 */
void UsageFault_Handler(void)
{
  RTT_LOG("UsageFault_Handler\n");
  while (1)
    ; 
}

/**
 * \brief Non-maskable interrupt handler
 * \note This interrupt that can't be disabled.
 *       If this triggers, something bad happened with the hardware.
 */
void NMI_Handler(void)
{
  RTT_LOG("NMI_Handler\n");
  while (1)
    ; 
}