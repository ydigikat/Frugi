# Understanding the BSP: A Simple Guide

## Introduction
When moving from other frameworks, such as Arduino, to other microcontroller programming, one of the first hurdles is understanding how to initialize and control the hardware. 

While high-level frameworks hide this complexity, working directly with STM32 microcontrollers requires understanding the Board Support Package (BSP).

The BSP package makes working with the STM32F411 hardware much easier for audio projects. It sets up all the complicated hardware stuff so you can focus on creating sounds. Instead of writing code that directly changes hardware registers, we use STM's built-in "LL drivers" - these are simple helper functions that make the code cleaner and easier to understand.

## Architecture Overview
The BSP has a modular design:

1. **Core Files (core_board.c/h)**: Contains common functionality shared by all supported boards
2. **Board-Specific Files (board.c/h)**: Contains board-specific configurations and pin definitions
3. **API:** Simple interface for your application to initialize the hardware and handle audio

## Call Flow
![Call Flow Diagram](assets/diagrams/bsp_call_flow.svg)

## Key Components of the BSP

### System Clock Configuration
The STM32F411 needs its clock system properly configured to run at the desired frequency (100MHz in this case). This is handled in ```clock_init()``` within ```core_board.c.```

```c
static void clock_init()
{
  /* Enable High-Speed External oscillator */
  LL_RCC_HSE_Enable();
  while (!LL_RCC_HSE_IsReady())
    ;
    
  /* Configure the PLL to multiply the clock */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, PLL_M, PLL_N, PLL_R);
  LL_RCC_PLL_Enable();
  /* ... etc ... */
}
```

Unlike Arduino where the clock is preset, here we:

1. Enable the external crystal oscillator
2. Configure the Phase-Locked Loop (PLL) to multiply the clock frequency
3. Set up various prescalers (dividers/multipliers) to derive clocks for different bus systems

### Floating-Point Unit (FPU)
For audio processing, floating-point math is often essential. The STM32F411 has a hardware FPU which must be explicitly enabled:

```c
static void fpu_init()
{
  SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
  __set_FPSCR(__get_FPSCR() | (1 << 24));
  __ISB();
  __DSB();
}
```
This is something Arduino users never need to worry about, but it's crucial for efficient DSP operations.

### GPIO Configuration
The BSP sets up GPIO pins for status LEDs, buttons, debug signals, and communication interfaces:

```c
bool gpio_init()
{
  /* LED */
  LL_GPIO_InitTypeDef led =
  {
    .Mode = LL_GPIO_MODE_OUTPUT,
    .OutputType = LL_GPIO_OUTPUT_PUSHPULL,
    .Pull = LL_GPIO_PULL_NO,
    .Pin = LL_GPIO_PIN_13,
  };
  /* ... etc ... */
}
```
While Arduino has simple pinMode() functions, STM32 requires more detailed configuration using structs.

### I2S Audio Interface
The heart of any digital audio system is the audio interface. This BSP uses I2S (Inter-IC Sound), a standard for connecting digital audio devices:

```c
static bool i2s_init()
{
  /* Configure GPIO pins for I2S */
  LL_GPIO_InitTypeDef io = {
    /* Pin configuration */
  };
  
  /* Configure I2S peripheral */
  LL_I2S_InitTypeDef i2s = {
    .AudioFreq = LL_I2S_AUDIOFREQ_44K,
    .DataFormat = LL_I2S_DATAFORMAT_32B,
    /* ... other settings ... */
  };
  /* ... etc ... */
}
```

The BSP also calculates the correct PLL settings for various sample rates (44.1kHz, 48kHz, 96kHz).

### DMA for Efficient Audio Transfers
Direct Memory Access (DMA) is used to transfer audio data without CPU intervention:

```c
static bool dma_i2s_init()
{
  LL_DMA_InitTypeDef dma_i2s = {
    .Channel = DMA_CHANNEL,
    .Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
    /* ... other settings ... */
  };
  /* ... etc ... */
}
```
DMA is vital for audio applications as it allows data transfer in the background while the CPU processes the next audio block.

### MIDI over UART
The BSP also includes UART configuration for MIDI communication at 31250 baud:
```c
static bool uart_init()
{
  /* Configure UART for MIDI (31250 baud, 8N1) */
  LL_USART_InitTypeDef usart = {
    .BaudRate = 31250,
    .DataWidth = LL_USART_DATAWIDTH_8B,
    /* ... other settings ... */
  };
  /* ... etc ... */
}
```
### Interrupts and Callbacks
The BSP uses interrupts to handle time-critical events:

```c
void DMA_IRQ_HANDLER(void)
{
  if (DMA->HISR & DMA_HISR_TCIF)
  {
    DMA->HIFCR = DMA_HIFCR_CTCIF;
    dae_ready_for_audio(1);
  }
  else
  {
    DMA->HIFCR = DMA_HIFCR_CHTIF;
    dae_ready_for_audio(0);
  }
}
```

This DMA interrupt handler calls back to the Digital Audio Engine (DAE) when a buffer needs filling, implementing a "ping-pong" buffer system common in audio processing.

### The Initialization Sequence
The BSP initialization follows this sequence:

1. Configure system clocks
2. Enable the FPU
3. Enable Flash cache for better performance
4. Initialize peripheral clocks
5. Configure GPIO pins
6. Initialize I2S interface
7. Set up DMA for I2S
8. Configure UART for MIDI

All this complexity is hidden behind a simple API call: ```board_init()```.

### Using the BSP in Your Project
To use the BSP in your own audio project:

1. Call ```board_init()``` at the start of your program
2. Implement the required callback functions:
    - ```dae_ready_for_audio()``` - Called when a buffer needs to be filled with audio
    - ```dae_midi_received()``` - Called when MIDI data is received

### Sample Rate Configuration

The audio system is started through a call to ```audio_start()```.

The BSP supports different sample rates by carefully configuring the I2S PLL:

```c
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock)
{
  /* Calculate the PLL speeds required */
  if (fsr == 48000)
  {
    LL_RCC_PLLI2S_ConfigDomain_I2S(LL_RCC_PLLSOURCE_HSE, I2S_PLL_M, I2S_48_N, I2S_48_R);
  }
  else if(fsr == 96000)
  {
    /* ... etc ... */
  }
}
```
The values for these dividers are carefully calculated to achieve precise sample rates from the system clock.

### Error Handling
The BSP includes exception handlers to catch hardware faults:
```c
void HardFault_Handler(void)
{ 
  RTT_LOG("HardFault_Handler\n");
  while (1)
    ;
}
```

These are very basic, since there isn't much the board can do except log the error (if possible) and sit waiting for a restart.

### Board-Specific Configuration
The board-specific files (board.c/h) define:

1. Pin mappings for the specific board
2. Clock configuration values
3. Hardware-specific initialization steps

This modular approach allows supporting multiple boards with the same core code.

### The Power of Debugger-Driven Understanding
While this walkthrough provides a high-level overview of the BSP code, there's simply no substitute for stepping through the code with a debugger to truly understand its operation. 

Text explanations can only go so far in conveying the dynamic nature of embedded systems. 

By using a debugger (like Visual Studio Code with appropriate extensions), you can watch registers change in real-time, observe how hardware peripherals respond to configuration, and see exactly how control flows through the initialization sequence. 

Set breakpoints at key functions like ```clock_init()```, ```i2s_init()```, and the interrupt handlers, then examine memory and peripheral registers to build a mental model of how the system operates. 

This hands-on approach will reveal subtleties and relationships that no walkthrough can fully capture, especially when timing-sensitive operations like DMA transfers and interrupt handling are involved. 

Even experienced embedded developers rely on debuggers to understand unfamiliar code bases – it's not just a tool for finding bugs but an essential learning aid.

### Conclusion
The BSP package handles all the complex hardware initialization needed for audio projects on the STM32F411. 

While Arduino and other frameworks hide this complexity, understanding the BSP gives you more control and opens up possibilities for advanced audio applications.

The carefully designed audio system with proper I2S configuration, DMA transfers, and interrupt handling provides a solid foundation for building digital synthesizers and other audio processing applications.