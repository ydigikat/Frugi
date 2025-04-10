# Digital Audio Engine (DAE)
## Making Real-Time Audio Processing Accessible
Have you ever wondered how digital synthesizers create sound in real-time without clicks, pops, or interruptions? 

The secret lies in efficiently managing audio streams between your hardware and your sound-generating code. 

That's exactly what the DAE does!

## What is the DAE?
This DAE is a lightweight audio processing framework designed specifically for makers and hobbyists building embedded synthesizers or audio effects. It handles all the messy details of getting audio data from your sound generator to your audio hardware smoothly and reliably.

## Why Do You Need a DAE?
Creating digital audio on microcontrollers is tricky business:

1. Real-time constraints: Audio must flow continuously with no interruptions
2. Limited resources: Microcontrollers have tight memory and CPU constraints
3. Complex coordination: MIDI input, parameter changes, and audio generation all need to happen simultaneously

Without a structured approach, you'll likely end up with glitchy audio, missed MIDI notes, or code that's hard to maintain. A DAE solves these problems with a clean architecture that separates your creative audio code from the hardware details.

## How this DAE Works: The Double-Buffer Magic
This DAE uses a technique called "double-buffering" (or ping-pong buffering) that's essential for smooth audio:

![Double-buffering diagram showing ping-pong buffers]

While one buffer is being transmitted to your audio hardware, the other is being filled with fresh audio samples. When transmission of the first buffer finishes, the roles swap instantly. This keeps your audio flowing without gaps!

Here's how it works step-by-step:

1. **Startup**: DAE initializes and starts the audio hardware
2. **Buffer Filling**: DAE fills the first buffer (let's call it "ping")
3. **Transmission**: The hardware starts sending ping's samples to your audio output
4. **Parallel Processing**: While ping is being sent, DAE fills the other buffer ("pong")
5. **Buffer Swap**: When ping is fully transmitted, the hardware automatically switches to pong
6. **Repeat**: While pong is being sent, DAE refills ping, and the cycle continues

This happens around 350 times per second, creating a seamless audio stream!



## The Audio Flow in Detail
When the DAE wakes up to fill a buffer, it follows this sequence:

1. **Process MIDI**: Any pending MIDI messages are handled first
2. **Update Parameters**: If any synth parameters were changed, they're updated
3. **Generate Audio**: Your synth code is called to fill a block of audio samples
4. **Format Conversion**: The samples are converted from floating-point to the format needed by the audio hardware
5. **Sleep**: DAE goes to sleep until the next buffer needs filling

## Integrating with Your Hardware
This DAE is designed to work with a variety of microcontrollers, but you'll need to connect it to your specific hardware. The board support package [(BSP)](bsp.md) takes care of this by implementing a few simple functions:

```c
// You implement this to start your audio hardware
void audio_start(int16_t audio_buffer[], size_t buf_len, uint32_t fsr, bool mclock);

// DAE provides these for your hardware interrupts to call
void dae_ready_for_audio(enum buffer_idx buffer_idx);
void dae_midi_received(uint8_t byte);
```

## Connecting Your Synthesizer (or other sample generator)
The DAE doesn't make sound itself – it's just the audio plumbing. You need to connect your synthesizer code by overriding these callback functions:

```c
// Initialize your synth with the correct sample rate
void dae_prepare_for_play(float sample_rate, size_t block_size, uint8_t *midi_channel);

// Handle parameter changes (from MIDI controllers, etc.)
void dae_update_parameters();

// This is where you generate your actual sound!
void dae_process_block(float *left, float *right, size_t block_size);

// Process incoming MIDI messages
void dae_handle_midi(struct midi_msg *msg);
```

## Parameter Management: Keeping Track of Knobs and Controls
One of DAE's handiest features is its parameter management system. 

This provides a central place to store all your synthesizer settings (like filter cutoff, oscillator waveforms, envelope times, etc.).

The parameter store:

- Automatically notifies your synth when parameters change
- Handles conversion between MIDI values (0-127) and normalized values (0.0-1.0)
- Makes it easy to create consistent control behavior

Using the parameter system is simple:

```c
// Set parameters (usually in response to MIDI CC messages)
param_set(PARAM_FILTER_CUTOFF, 0.75f);        // Using normalized value
param_set_midi(PARAM_RESONANCE, midi_value);  // Using MIDI value (0-127)

// Get parameters (usually in your audio processing code)
float cutoff = param_get(PARAM_FILTER_CUTOFF);
```

## MIDI Handling Made Easy
MIDI (Musical Instrument Digital Interface) is how your synth communicates with keyboards, sequencers, and controllers. DAE includes a complete MIDI parser that handles:

- Note on/off events
- Control changes (knob turns, slider movements)
- Pitch bend
- Programme change
- Running status and other MIDI protocol details

You don't need to understand the MIDI protocol - just implement the ```dae_handle_midi()``` function to respond to parsed messages!

## Try the Test Tone
This DAE includes a simple test tone generator that produces a 440 Hz sine wave. This is great for verifying your audio hardware is working correctly before diving into your own sound generation code.

This walkthrough breaks down the important parts of the code step by step, perfect if you're learning or want to customize things for your own projects!

## Want to See How It All Works?
Curious about what's happening under the hood? Check out our detailed explanation of the DAE and MIDI code:

- [Dig Deeper into the DAE](dae_code.md)
- [Poke around the params](params_code.md)
- [And what about the MIDI?](midi_code.md)

## Summary
The DAE gives you:

- Professional-quality audio handling with minimal code
- A clean separation between hardware and sound generation
- Built-in parameter management and MIDI parsing
- A solid foundation for your audio projects

Whether you're building a simple effect pedal or a complex polyphonic synthesizer, DAE provides the structure you need to focus on the creative parts rather than the plumbing.
Happy sound making!