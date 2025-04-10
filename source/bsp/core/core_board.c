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

/* Calls to DAE */
extern void dae_ready_for_audio(uint8_t buffer_idx); /* This tells the Digital Audio Engine that a buffer is ready */
extern void dae_midi_received(uint8_t byte);         /* This passes MIDI data to the DAE when received */

/**
 * Sets up the main system clock
 *
 * This configures the STM32F4's clock system:
 * - Enables the external high-speed clock (HSE)
 * - Sets up the Phase Locked Loop (PLL) for the desired CPU frequency
 * - Configures various bus dividers
 *
 * In plain English: This gets our chip running at the right speed!
 */
static void clock_init()
{
  LL_RCC_HSE_Enable(); // Turn on the external crystal oscillator & wait until stable.
  while (!LL_RCC_HSE_IsReady())
    ;

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);         // Enable power management clock
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1); // Set voltage scaling for best performance
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);                   // Flash memory needs wait states at high CPU speeds

  // Configure the main PLL - this is like tuning your CPU to the right frequency
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, PLL_M, PLL_N, PLL_R);
  LL_RCC_PLL_Enable(); // Start the PLL and wait until it stabilises
  while (!LL_RCC_PLL_IsReady())
    ;

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1); // No division for the main system clock

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL); // Use the PLL as our main clock source
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {
  }; // Wait until the system is running on the PLL

  // Set up the peripheral bus clocks
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2); // APB1 runs at half speed (needed for certain peripherals)
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1); // APB2 runs at full speed
  SystemCoreClock = FREQ;                     // Tell the system what frequency we're running at
}

/**
 * Enables the Floating Point Unit (FPU)
 *
 * Audio DSP needs floating point math, and this chip has hardware acceleration for it.
 * This function turns it on so we can do audio calculations way faster!
 */
static void fpu_init()
{
  // Enable CP10 and CP11 coprocessors in CPACR register
  SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
  // Set FPSCR register and make sure the setting is committed
  __set_FPSCR(__get_FPSCR() | (1 << 24));
  __ISB(); // Instruction Synchronization Barrier
  __DSB(); // Data Synchronization Barrier
}

/**
 * Sets up the UART for MIDI input
 *
 * MIDI runs at 31,250 baud - this configures the UART for that specific speed
 * and sets it to only receive data (no transmit needed for most synth projects).
 *
 * Returns true if successful, false if initialization failed.
 */
static bool uart_init()
{
  /* RX only - we're just receiving MIDI data, not sending any */
  LL_GPIO_InitTypeDef gpio =
      {
          .Mode = LL_GPIO_MODE_ALTERNATE,   // The pin is connected to the UART peripheral
          .Alternate = UART_AF,             // Alternate function number for UART
          .Pin = UART_RX_PIN,               // Which pin to use for UART RX
          .Speed = LL_GPIO_SPEED_FREQ_HIGH, // Fast operation mode
          .Pull = LL_GPIO_PULL_UP};         // Use internal pull-up resistor

  if (LL_GPIO_Init(UART_RX_PORT, &gpio) != SUCCESS)
  {
    return false; // If GPIO setup fails, abort
  }

  /* USART config  31250, 8N1 - these are the exact MIDI specs */
  LL_USART_InitTypeDef usart =
      {
          .BaudRate = 31250,                           // MIDI standard speed
          .OverSampling = LL_USART_OVERSAMPLING_16,    // Standard oversampling
          .DataWidth = LL_USART_DATAWIDTH_8B,          // 8 bits per character
          .Parity = LL_USART_PARITY_NONE,              // No parity bit
          .StopBits = LL_USART_STOPBITS_1,             // 1 stop bit
          .TransferDirection = LL_USART_DIRECTION_RX}; // Only receive, no transmit

  if (LL_USART_Init(UART, &usart) != SUCCESS)
  {
    return false; // If UART setup fails, abort
  }

  // Set up interrupt handling so we get notified when MIDI data arrives
  NVIC_EnableIRQ(UART_IRQN);      // Enable the UART interrupt in the NVIC
  NVIC_SetPriority(UART_IRQN, 6); // Medium-high priority for MIDI (it's timing-sensitive but not as critical as audio)

  LL_USART_EnableIT_RXNE(UART); // Enable the "Receive Not Empty" interrupt

  /* Enable UART */
  LL_USART_Enable(UART); // Turn on the UART
  while (!LL_USART_IsEnabled(UART))
    ; // Wait until it's running

  return true;
}

/**
 * Sets up the I2S interface for audio output
 *
 * I2S is a digital audio protocol that connects to DACs like PCM5102, WM8731, etc.
 * This configures the STM32F4's I2S peripheral to output digital audio data.
 *
 * Returns true if successful, false if initialization failed.
 */
static bool i2s_init()
{
  // Common GPIO settings for all I2S pins
  LL_GPIO_InitTypeDef io =
      {
          .Speed = LL_GPIO_SPEED_FREQ_HIGH, // Fast mode for clean audio signals
          .Pull = LL_GPIO_PULL_DOWN,        // Pull-down when inactive
          .Mode = LL_GPIO_MODE_ALTERNATE,   // Connected to the I2S peripheral
          .Alternate = I2S_AF,              // Alternate function number for I2S
      };

  // Set up Master Clock pin (MCK) - only needed for some DACs
  io.Pin = I2S_MCK_PIN;
  if (LL_GPIO_Init(I2S_MCK_PORT, &io) != SUCCESS)
  {
    return false;
  }

  // Set up Serial Clock pin (SCK, aka BCLK) - the bit clock
  io.Pin = I2S_SCK_PIN;
  if (LL_GPIO_Init(I2S_SCK_PORT, &io) != SUCCESS)
  {
    return false;
  }

  // Set up Serial Data Out pin (SDO, aka SD) - the actual audio data
  io.Pin = I2S_SDO_PIN;
  if (LL_GPIO_Init(I2S_SDO_PORT, &io) != SUCCESS)
  {
    return false;
  }

  // Set up Word Select pin (WS, aka LRCK) - indicates left/right channel
  io.Pin = I2S_WS_PIN;
  if (LL_GPIO_Init(I2S_WS_PORT, &io) != SUCCESS)
  {
    return false;
  }

  /* Configure I2S peripheral (partial - final configuration in audio_start) */
  LL_I2S_InitTypeDef i2s =
      {
          .AudioFreq = LL_I2S_AUDIOFREQ_44K,        /* This will be changed in audio_start() function */
          .ClockPolarity = LL_I2S_POLARITY_LOW,     // Clock is low when idle
          .DataFormat = LL_I2S_DATAFORMAT_32B,      // 32-bit audio data format
          .MCLKOutput = LL_I2S_MCLK_OUTPUT_DISABLE, // Temporarily disable MCLK until fully configured
          .Mode = LL_I2S_MODE_MASTER_TX,            // We're the clock master, transmitting only
          .Standard = LL_I2S_STANDARD_PHILIPS};     // Standard I2S format (Philips)

  if (LL_I2S_Init(I2S, &i2s) != SUCCESS)
  {
    return false;
  }

  /* I2S is started in board_start() */

  return true;
}

/**
 * Sets up the DMA for I2S audio output
 *
 * DMA (Direct Memory Access) lets us transfer audio data without CPU involvement.
 * This is crucial for audio - the CPU can do sound processing while DMA
 * handles the constant stream of data to the I2S port autonomously.
 *
 * Returns true if successful, false if initialization failed.
 */
static bool dma_i2s_init()
{
  LL_DMA_InitTypeDef dma_i2s =
      {
          .Channel = DMA_CHANNEL,                               // Which DMA channel to use
          .Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,       // Going from RAM to the I2S peripheral
          .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,   // I2S data register stays the same
          .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,     // Step through the audio buffer
          .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_HALFWORD, // I2S takes 16-bit chunks
          .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_HALFWORD, // Audio data is 16-bit
          .Mode = LL_DMA_MODE_CIRCULAR,                         // Keep going around the buffer forever
          .Priority = LL_DMA_PRIORITY_HIGH,                     // Audio needs high priority
          .FIFOMode = LL_DMA_FIFOMODE_DISABLE};                 // Direct transfer, no FIFO buffering

  if (LL_DMA_Init(DMA, DMA_STREAM, &dma_i2s) != SUCCESS)
  {
    return false;
  }

  // Enable interrupts at half-transfer (HT) and transfer-complete (TC)
  // These will tell us when to fill the next half of the buffer (ping-pong buffering)
  LL_DMA_EnableIT_HT(DMA, DMA_STREAM); // Half-transfer interrupt (first half done)
  LL_DMA_EnableIT_TC(DMA, DMA_STREAM); // Transfer-complete interrupt (second half done)

  // Configure the interrupt handling
  NVIC_SetPriority(DMA_IRQN, 10); // High priority - audio is time-critical!
  NVIC_EnableIRQ(DMA_IRQN);       // Enable DMA interrupts in the NVIC

  // Connect the DMA to the I2S peripheral
  LL_SPI_EnableDMAReq_TX(I2S);

  /* DMA is started in board_start() */

  return true;
}

/**
 * Main board initialization function
 *
 * This sets up all the hardware needed for our synth:
 * - System clocks
 * - Floating point unit
 * - Flash cache for faster code execution
 * - GPIO pins
 * - I2S audio output
 * - DMA for buffer transfers
 * - UART for MIDI input
 *
 * Returns true if everything initialized correctly, false if anything failed.
 */
bool core_board_init(void)
{
  clock_init(); // Set up system clocks
  fpu_init();   // Enable floating point hardware

  /* Enable ART flash cache subsystem - makes code run faster */
  LL_FLASH_EnablePrefetch();  // Prefetch instructions for speed
  LL_FLASH_EnableInstCache(); // Cache instructions
  LL_FLASH_EnableDataCache(); // Cache data

  enable_peripheral_clocks(); // Turn on clocks for the peripherals we'll use

  // Initialize all the peripherals we need, checking for errors
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

  return true; // Everything initialized correctly!
}

/**
 * audio_start
 * \brief called by the DAE to start the audio hardware (callback)
 * \note only set mclock true if you are using a master clock, otherwise the WS/BCLK rates will calculate incorrectly.
 * \param audio_buffer the audio buffer, this is split by the DAE into ping/pong halves.
 * \param buf_len the length of the audio buffer (32 bit words, interleaved stereo L+R)
 * \param fsr the sample rate (44100, 48000, 96000)
 * \param mclock true if you need a master clock (see note)
 *
 * This is the function that actually starts the audio flowing!
 * It's called by the Digital Audio Engine when it's ready to start making sound.
 * The sample rate can be changed on the fly by calling this with different fsr values.
 */
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock)
{
  /* Set DMA transfer buffer */
  LL_DMA_SetDataLength(DMA, DMA_STREAM, buf_len); // Tell DMA how many samples to transfer
  // Set up the memory-to-peripheral transfer addresses
  LL_DMA_ConfigAddresses(DMA, DMA_STREAM, (uint32_t)audio_buffer, LL_SPI_DMA_GetRegAddr(I2S), LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  /* Calculate the PLL speeds required for different sample rates */
  // I2S needs a specific clock based on sample rate - we configure that here
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
    // Default to 44.1kHz (CD quality)
    LL_RCC_PLLI2S_ConfigDomain_I2S(LL_RCC_PLLSOURCE_HSE, I2S_PLL_M, I2S_44_N, I2S_44_R);
  }

  /* Re-enable PLLI2S */
  LL_RCC_PLLI2S_Enable(); // Turn on the I2S clock generator
  while (!LL_RCC_PLLI2S_IsReady())
    ; // Wait until it's stable

  /* Update I2S peripheral with new frequency */
  LL_I2S_InitTypeDef i2s =
      {
          .AudioFreq = fsr, // Set the actual sample rate we want to use
          .ClockPolarity = LL_I2S_POLARITY_LOW,
          .DataFormat = LL_I2S_DATAFORMAT_24B,                                           // 24-bit audio data format (better quality than 16-bit)
          .MCLKOutput = mclock ? LL_I2S_MCLK_OUTPUT_ENABLE : LL_I2S_MCLK_OUTPUT_DISABLE, // Master clock only if needed
          .Mode = LL_I2S_MODE_MASTER_TX,
          .Standard = LL_I2S_STANDARD_PHILIPS};

  if (LL_I2S_Init(I2S, &i2s) != SUCCESS)
  {
    return; // If I2S re-init fails, just return (not much we can do)
  }

  /* Enable I2S */
  LL_I2S_Enable(I2S); // Turn on the I2S peripheral
  while (!LL_I2S_IsEnabled(I2S))
    ; // Wait until it's running

  /* Enable DMA */
  LL_DMA_EnableStream(DMA, DMA_STREAM); // Start the DMA engine
  while (!LL_DMA_IsEnabledStream(DMA, DMA_STREAM))
    ; // Wait until it's running

  /* Enable UART */
  LL_USART_Enable(UART); // Make sure UART is running (for MIDI)
  while (!LL_USART_IsEnabled(UART))
    ; // Wait until it's active
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
  dae_midi_received(UART->DR); // Send the received MIDI byte to the DAE
}

/**
 * Hard fault exception handler
 *
 * If the processor hits a serious error, this gets called.
 * It logs the error and hangs the system (better than random behavior).
 */
void HardFault_Handler(void)
{
  RTT_LOG("HardFault_Handler\n");
  while (1)
    ; // Hang the system - we can't recover from this
}

/**
 * Memory management fault handler
 *
 * Called on memory protection errors.
 * Generally means trying to access memory we shouldn't.
 */
void MemManage_Handler(void)
{
  RTT_LOG("MemManage_Handler\n");
  while (1)
    ; // Hang the system
}

/**
 * Bus fault exception handler
 *
 * Called on errors during memory transfers.
 * Often means trying to access hardware that isn't there.
 */
void BusFault_Handler(void)
{
  RTT_LOG("BusFault_Handler\n");
  while (1)
    ; // Hang the system
}

/**
 * Usage fault exception handler
 *
 * Called on various CPU usage errors like trying to execute invalid instructions.
 */
void UsageFault_Handler(void)
{
  RTT_LOG("UsageFault_Handler\n");
  while (1)
    ; // Hang the system
}

/**
 * Non-maskable interrupt handler
 *
 * This is the "emergency exit" interrupt that can't be disabled.
 * If this triggers, something seriously bad happened with the hardware.
 */
void NMI_Handler(void)
{
  RTT_LOG("NMI_Handler\n");
  while (1)
    ; // Hang the system
}