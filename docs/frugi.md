# Frugi: A DIY VA Synth for Embedded Systems
Welcome to Frugi, an open-source virtual analog (VA) synthesizer designed for microcontrollers! Whether you're coming from Arduino or just starting with embedded audio, this project will help you understand how digital synthesizers work and how to build one yourself.

### What is Frugi?
Frugi is a polyphonic (multi-voice) virtual analog synthesizer that runs on the STM32F411 microcontroller. It features:

- Classic subtractive synthesis architecture
- Multiple oscillator waveforms (triangle, saw, pulse)
- Multimode resonant filter
- Envelopes for amplitude and modulation
- A unique LFO system inspired by the vintage Yamaha CS20M
- Noise generator for percussion and special effects
- MIDI input for playing notes and controlling parameters

![Block Diagranm](assets/diagrams/block.svg)

### How Frugi Works
Frugi processes audio in small chunks (or "blocks") of 128 samples at a time at a sample rate of 48kHz. This gives about 375 blocks per second - fast enough for real-time audio but not so fast that the CPU can't keep up with other tasks.

### Signal Flow
Here's how sound is created and processed in Frugi:

```
               ┌───────────────┐
               │ frugi_synth   │ (Controls everything)
               └───────┬───────┘
                       │
                       ▼
               ┌───────────────┐
               │ frugi_voice   │ (Manages multiple notes)
               └───────┬───────┘
                       │
           ┌───────────┴───────────┐
           │                       │
┌──────────▼─────────┐    ┌────────▼────────┐
│ Audio Rate         │    │ Control Rate    │
├────────────────────┤    ├─────────────────┤
│  - frugi_osc       │    │  - frugi_env_gen│
│  - frugi_noise     │    │  - frugi_lfo    │
└──────────┬─────────┘    └────────┬────────┘
           │                       │
           ▼                       ▼
 ┌─────────────────┐     ┌─────────────────┐
 │  frugi_filter   │ ◄── │ Modulation      │
 └────────┬────────┘     └─────────────────┘
          │
          ▼
 ┌─────────────────┐
 │   frugi_amp     │
 └────────┬────────┘
          │
          ▼
     Audio Output
```
When you press a key on your MIDI keyboard:

1. The synthesizer finds an available voice (or steals the oldest one if all are in use)
2. Two oscillators generate raw waveforms (like sawtooth or square waves)
3. The waveforms and noise generator (if enabled) are mixed together
4. This mixed signal passes through a filter that can be modulated
5. The amplitude envelope shapes the volume over time
6. The resulting sound comes out of your speakers!

## The Building Blocks of Frugi
### Oscillators
Each voice has two oscillators that generate the basic sound. You can control:

1. Waveform: Choose between triangle, sawtooth, and pulse waves
2. Tuning: Set octave, semitone, and fine-tune (cents) for each oscillator
3. Pulse Width: For the pulse wave, adjust how "square" it sounds (from narrow to wide)
4. Level: How loud each oscillator is in the mix
5. Modulation: How much the LFO and envelope affect pitch

The oscillators use special techniques (like BLEP: Band-Limited step) to reduce the harsh digital artifacts that would otherwise occur with these waveforms

### Noise Generator
The noise module adds white or pink noise to the mix - perfect for creating percussion sounds, wind effects, or adding "breathiness" to sounds.

### Filter
The filter is where Frugi gets much of its character. It removes certain frequencies from the sound, creating the classic "sweeping" effect heard in electronic music.

1. Cutoff: Controls which frequencies pass through
2. Resonance: Emphasizes frequencies around the cutoff point
3. Mode: Choose from low-pass, band-pass, and high-pass in 2-pole or 4-pole configurations
4. Saturation: Adds warm distortion at high levels
5. Envelope Amount: How much the envelope opens and closes the filter
6. LFO Modulation: Creates rhythmic filter movement

### Envelopes
Envelopes shape how sounds evolve over time. Frugi has two envelope generators:

1. Amplitude Envelope: Controls the volume of the sound with ADSR (Attack, Decay, Sustain, Release)
2. Modulation Envelope: Can be routed to pitch or filter cutoff

Each envelope stage does something different:

- Attack: How quickly the sound fades in when you press a key
- Decay: How quickly it falls to the sustain level
- Sustain: The level maintained while the key is held
- Release: How quickly the sound fades when you release the key

Envelopes can also respond to how hard you press the keys (velocity) and which notes you play.

Frugi's envelopes are loosely modelled on the Curtis analogue R/C envelope generator.

### The LFO (Low-Frequency Oscillator)
Frugi's LFO is special! Based on the vintage Yamaha CS20M, it generates all waveforms simultaneously. 

Each destination (oscillator, filter, amplifier) can use a different waveform and depth, but they all share the same rate.

Available LFO waveforms:

- Triangle
- Sawtooth
- Reverse sawtooth
- Square
- Random (sample & hold)

The LFO can be free-running or triggered when you play a note.

### Amplifier
The final stage controls the overall volume and can be modulated by the LFO for tremolo effects.

### Voice Management
Frugi can play multiple notes at once (polyphony). When you play more notes than there are voices available, it uses a "voice stealing" algorithm to determine which currently playing note to cut off.

### Plenty of Parameters

For a more detailed dive into the parameters:

[Tweakability!](frugi_params.md)