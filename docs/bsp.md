# BSP

## Getting Started with the Board Support Package

Think of the Board Support Package (BSP) as the friendly translator between your cool audio code and the actual hardware.

It takes care of all those tedious hardware tasks on the STM32F411 board - setting up clocks, configuring pins, talking to the audio codec, and moving sound data around automatically.

The best part? Once this is set up, you can focus on the fun stuff - creating awesome sounds with the Digital Audio Engine (DAE) without worrying about the nitty-gritty hardware details.

If you're coming from the Arduino world, this is similar to how the Arduino core handles all the messy microcontroller stuff so you can just call simple functions like analogWrite() or digitalRead().

The real magic is that if you ever want to move your synth project to a different board in the future, you'd only need to swap out this BSP layer while keeping all your sound-making code exactly the same.

Even if you've never touched an STM32 board before, don't worry! The BSP gives you simple ways to control the hardware without needing to dive into complicated datasheets or understanding every single register.

## What's in the Box?

- **Simple Setup:** Gets your system clock up and running without the headache

- **Audio Math Boost:** Turns on the floating-point hardware with special tweaks for smoother sound processing

- **Basic Controls Working:** Makes LEDs light up, buttons respond, and connects test equipment with minimal fuss

- **CD-Quality Sound (and Beyond):** Supports standard audio rates (44.1kHz like CDs, 48kHz like DVDs) and even high-res 96kHz for the audio enthusiasts

- **Hands-Free Audio:** Uses DMA to move audio data automatically, freeing up your processor for the cool stuff

- **MIDI Ready:** Connects to MIDI keyboards and controllers right out of the box

If you happen to have a J-Link debugging tool (nice, but totally optional):

- **Peek Under the Hood:** See what's happening inside your code in real-time with RTT

- **Speed Detective:** Measure exactly how fast (or slow!) different parts of your code are running

## Supported Boards
I've tested this code on these boards (the ones I personally use):

- STM32F411 (Blackpill generic)
- STM32F411 Discovery

The BSP supports these out of the box.

## The Simple Commands You'll Need

### Getting Started

```c
bool board_init(void);
```
Sets up everything your board needs in one go! Configures clocks, pins, audio, and all that technical stuff. Returns true if everything's ready to go, false if something went wrong.

### Making Noise
```c
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr);
```
Kicks off the audio output with your sound data:

- ```audio_buffer```: Where your sound data lives (left/right channels alternating)
- ```buf_len```: How many samples are in your buffer
- ```fsr```: Pick your quality: 44100 (CD quality), 48000 (standard digital)

### Functions Your Audio Engine Needs to Provide
```c
void dae_ready_for_audio(uint8_t buffer_idx);
```
The board calls this when it's after more audio data:

- ```buffer_idx```: Tells you which half of the buffer to fill (0 or 1)

```c
void dae_midi_received(uint8_t byte);
```
The board calls this whenever it gets MIDI data from a keyboard or controller:

- ```byte```: The MIDI message that just arrived

### Debug Tools
If you're using a Segger J-Link probe and have RTT_ENABLED defined, you get these handy debugging tools:

```c
RTT_LOG(fmt, ...);            // Print messages to your debug console
RTT_LOG_FLOAT(fmt, ...);      // Same, but works with decimal numbers too
RTT_ASSERT(expr);             // Checks if something's true (in DEBUG mode)

// Timing tools to see how fast your code runs
DWT_INIT();                   // Start the stopwatch
DWT_CLEAR();                  // Reset the stopwatch
DWT_OUTPUT(msg);              // See how much time passed
```

### Controlling LEDs and Buttons
```c
USR_LED_ON();                 // Light up the onboard LED
USR_LED_OFF();                // Turn off the LED
READ_USR_BTN();               // Check if the button is pressed

// For connecting test equipment (D0-D7)
D0_SET(); D0_CLEAR();         // Turn on/off debug pin 0
D1_SET(); D1_CLEAR();         // Turn on/off debug pin 1
// ... and so on up to pin 7
```

## How the Audio Magic Happens
Think of your audio system like a relay race with two runners. 

While one runner (DMA) is delivering sound data to the speakers, the other runner (your audio engine) is getting the next batch of sounds ready.

The system uses a technique called "double-buffering" - imagine a single big audio buffer split into two halves:

```
[L0][R0][L1][R1]...[Ln][Rn] | [Ln+1][Rn+1]...[L2n][R2n]
|------ First Half ------|  |------ Second Half ------|
```

Here's how it works:

1. Your code fills the first half with cool sounds
2. The DMA starts playing those sounds through the speakers
3. While that's playing, your code fills the second half with more sounds
4. When the first half finishes playing, the system calls dae_ready_for_audio(0) to say "Hey, the first half is empty now, fill it up again!"
5. While you're refilling the first half, the second half is playing
6. When the second half finishes, you get dae_ready_for_audio(1) to fill that half again

This back-and-forth continues forever, creating a smooth stream of sound without your processor having to babysit the transfer of every sample.

Each [L][R] pair is one stereo sample - L for left speaker, R for right speaker.

### Speeding Things Up for Better Sound

- **Float Math Turbocharge**: We've tweaked the floating-point math unit to instantly convert super tiny numbers to zero. This might sound like a small thing, but it makes effects like reverb and filter feedback signals run WAY faster!

- **Hands-Free Audio Transfer**: The DMA (Direct Memory Access) works in "circular mode" - which means once you set it up, it just keeps going round and round through your buffer without needing your CPU to manage it.

- **Smart Priorities**: The system makes sure that generating your audio takes precedence over handling MIDI messages. This means even when you're playing a complex piece on your MIDI keyboard, the sound output stays smooth and glitch-free.

## Want to See How It All Works?
Curious about what's happening under the hood? Check out our detailed explanation of the BSP code:

[Peek Behind the Curtain](bsp_code.md)

This walkthrough breaks down the important parts of the code step by step, perfect if you're learning or want to customize things for your own projects!


## License
MIT License - Copyright (c) 2025 YDigiKat

Permission to use, copy, modify, and/or distribute this code for any purpose with or without fee is hereby granted, provided the above copyright notice and this permission notice appear in all copies.
