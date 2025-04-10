# Frugi: Understanding the Synthesizer Parameters
One of the best things about Frugi is how tweakable it is! Let's break down all the parameters you can adjust and what each one actually does to your sound.

### Understanding Frugi's Parameter System
Frugi uses a specific system to handle all its knobs and settings in a consistent way. Here's how it works in plain language:

#### The Basic Idea

1. **Everything is stored as a number between 0.0 and 1.0**. This is called "unipolar normalized form"
It makes all parameters work the same way internally
    - 0.0 = minimum value 
    - 1.0 = maximum value
2. **MIDI Controllers set values 0-127**. 
    - When you turn a knob on your MIDI controller, it sends a value in this range
    - Frugi automatically converts this to normalized form (0-127 → 0.0-1.0)

3. **The synth modules need different kinds of values**
    - Some need whole numbers (like selecting a waveform)
    - Some need frequencies (like filter cutoff)
    - Some need time values (like envelope attack)

4. **Special conversion formulas transform normalized values to what's needed**
    - This happens automatically inside each module

### Example 1: Selecting a Waveform
When you want to select the oscillator waveform:

1. Your MIDI controller sends a value (let's say 85)
2. Frugi converts this to normalized form: 85/127 = ~0.67
3. The oscillator module needs a whole number (0=Triangle, 1=Saw, 2=Pulse)
4. It uses PARAM_TO_INT(0.67, 0, 2), which gives 1 (Sawtooth)

```MIDI CC (85) → Normalized (0.67) → Integer selection (1 = Sawtooth)```

### Example 2: Setting Filter Cutoff
When adjusting the filter cutoff:

1. Your MIDI controller sends a value (let's say 64)
2. Frugi converts this to normalized form: 64/127 = 0.5
3. The filter needs a frequency value between 80Hz and 18000Hz
4. It uses PARAM_TO_EXP(0.5, 80, 18000), which gives ~1200Hz

```MIDI CC (64) → Normalized (0.5) → Exponential frequency (1200Hz)```

This uses an exponential curve because our ears perceive pitch logarithmically. A value of 0.5 isn't halfway between 80Hz and 18000Hz - it's more musical this way.

### Example 3: Setting Envelope Attack Time
When setting how quickly a sound fades in:

1. Your MIDI controller sends a value (let's say 32)
2. Frugi converts this to normalized form: 32/127 = ~0.25
3. The envelope needs a time value between 1ms and 10000ms
4. It uses PARAM_TO_POWER(0.25, 1, 10000, 1.5), giving ~100ms

```MIDI CC (32) → Normalized (0.25) → Power curve time value (100ms)```

The power curve makes small adjustments at low values more precise (for quick attacks) while still allowing very long times at higher values.

### Example 4: Choosing a Mode from a List
When selecting the LFO mode (Trigger or Free-running):

1. Your MIDI controller sends a value (let's say 0 or 127)
2. Frugi converts this to normalized form: 0/127 = 0.0 or 127/127 = 1.0
3. The LFO needs to know which mode (0=Trigger, 1=Free)
4. It uses PARAM_TO_INT(value, 0, 1), giving either 0 or 1

```MIDI CC (0 or 127) → Normalized (0.0 or 1.0) → Integer selection (0 or 1)```

### The Bottom Line
This system might seem complicated at first, but it provides big advantages:

- Consistent Interface: All parameters are controlled the same way
- Appropriate Scaling: Each parameter can use the most musical/useful curve for its purpose
- Future-Proof: If you decide to control Frugi with something other than MIDI later, everything still works

As a user, you don't need to worry about these conversions - just know that turning your MIDI controller knobs will produce musically useful changes to the sound!

## All Frugi Synthesizer Parameters

Note on Parameter Ranges: The "Range" column shows the actual useful values after conversion from normalized form. 

Internally, all parameters are stored in normalized form (0.0-1.0), and MIDI controllers interact with them using values from 0-127. 

The ranges shown represent what these normalized values map to when used internally by the synthesizer.

### Waveform Selection Example (OSC1_WAVE)

| Value Type |Triangle|Sawtooth|Pulse|
| -----------|--------|--------|-----|
| MIDI CC (0-127) | 0-42|43-84|85-127|
|Normalized (0.0-1.0)|0.0-0.33|0.34-0.66|0.67-1.0|
|Module Integer| (0-2)|0|1|2|

When you turn a knob on your MIDI controller, the value gets converted through these stages automatically. The synth engine only cares about the final integer value (0, 1, or 2) to know which waveform to generate.


### Oscillator Parameters

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_OSC1_WAVE` / `FRUGI_OSC2_WAVE` | Waveform | 0-2 | Selects oscillator waveform: 0=Triangle, 1=Sawtooth, 2=Pulse |
| `FRUGI_OSC1_OCTAVE` / `FRUGI_OSC2_OCTAVE` | Octave | -2 to +2 | Shifts pitch by whole octaves |
| `FRUGI_OSC1_SEMI` / `FRUGI_OSC2_SEMI` | Semitone | -11 to +11 | Fine-tunes in semitones (half-steps) |
| `FRUGI_OSC1_CENTS` / `FRUGI_OSC2_CENTS` | Cents | -99 to +99 | Super-fine tuning (1/100th of a semitone) |
| `FRUGI_OSC1_PULSE_WIDTH` / `FRUGI_OSC2_PULSE_WIDTH` | Pulse Width | 0.02-0.98 | Adjusts pulse wave shape (0.5=square) |
| `FRUGI_OSC1_LFO_FUNCTION` / `FRUGI_OSC2_LFO_FUNCTION` | LFO Wave | 0-4 | Selects which LFO waveform modulates pitch |
| `FRUGI_OSC1_LFO_DEPTH` / `FRUGI_OSC2_LFO_DEPTH` | LFO Amount | 0.0-1.0 | How much LFO affects pitch |
| `FRUGI_OSC1_ENV_DEPTH` / `FRUGI_OSC2_ENV_DEPTH` | Envelope Amount | 0.0-1.0 | How much envelope affects pitch |
| `FRUGI_OSC1_LEVEL` / `FRUGI_OSC2_LEVEL` | Level | 0.0-1.0 | Oscillator volume in the mix |

### Noise Generator

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_NOISE_LEVEL` | Noise Level | 0.0-1.0 | Amount of noise in the mix |
| `FRUGI_NOISE_TYPE` | Noise Type | 0-1 | 0=White Noise, 1=Pink Noise |

### Filter Section

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_FILTER_MODE` | Filter Type | 0-5 | 0=LPF2, 1=BPF2, 2=HPF2, 3=LPF4, 4=BPF4, 5=HPF4 |
| `FRUGI_FILTER_CUTOFF` | Cutoff Frequency | 80Hz-18kHz | Sets filter's cutoff frequency |
| `FRUGI_FILTER_RESONANCE` | Resonance | 0.0-4.0 | Boosts frequencies around cutoff point |
| `FRUGI_FILTER_SATURATION` | Saturation | 0.0-5.0 | Adds warm distortion to filter output |
| `FRUGI_FILTER_ENV_AMOUNT` | Envelope Amount | 0.0-1.0 | How much envelope affects cutoff |
| `FRUGI_FILTER_LFO_FUNCTION` | LFO Wave | 0-4 | Selects which LFO waveform modulates filter |
| `FRUGI_FILTER_LFO_DEPTH` | LFO Amount | 0.0-1.0 | How much LFO affects cutoff |
| `FRUGI_FILTER_LFO_RANGE` | LFO Range | 0-1 | 0=Normal (4x), 1=Extreme (10x) modulation |
| `FRUGI_FILTER_NOTE_TRACK` | Note Tracking | 0-1 | 0=Off, 1=On (cutoff follows note pitch) |

### Modulation Envelope

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_MOD_ENV_ATTACK` | Attack | 1ms-10s | Time to reach full level when key pressed |
| `FRUGI_MOD_ENV_DECAY` | Decay | 2ms-15s | Time to fall to sustain level after attack |
| `FRUGI_MOD_ENV_SUSTAIN` | Sustain | 0.0-1.0 | Level held while key remains pressed |
| `FRUGI_MOD_ENV_RELEASE` | Release | 2ms-30s | Time to fade to zero after key released |
| `FRUGI_MOD_ENV_MODE` | Mode | 0-3 | 0=Normal, 1=Biased, 2=Inverted, 3=Biased Inverted |
| `FRUGI_MOD_ENV_VEL_SENS` | Velocity Sensitivity | 0-1 | 0=Off, 1=On (velocity affects attack) |
| `FRUGI_MOD_ENV_NOTE_TRACK` | Note Tracking | 0-1 | 0=Off, 1=On (higher notes = faster envelope) |

### Amplitude Envelope

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_AMP_ENV_ATTACK` | Attack | 1ms-10s | Time to reach full volume when key pressed |
| `FRUGI_AMP_ENV_DECAY` | Decay | 2ms-15s | Time to fall to sustain level after attack |
| `FRUGI_AMP_ENV_SUSTAIN` | Sustain | 0.0-1.0 | Volume level held while key remains pressed |
| `FRUGI_AMP_ENV_RELEASE` | Release | 2ms-30s | Time to fade to zero after key released |
| `FRUGI_AMP_ENV_VEL_SENS` | Velocity Sensitivity | 0-1 | 0=Off, 1=On (velocity affects volume) |
| `FRUGI_AMP_ENV_NOTE_TRACK` | Note Tracking | 0-1 | 0=Off, 1=On (higher notes = faster envelope) |

### Amplifier

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_AMP_VOLUME` | Volume | 0.0-1.0 | Master volume for each voice |
| `FRUGI_AMP_PAN` | Pan | 0.0-1.0 | Stereo position (0=left, 0.5=center, 1=right) |
| `FRUGI_AMP_LFO_FUNCTION` | LFO Wave | 0-4 | Selects which LFO waveform modulates volume |
| `FRUGI_AMP_LFO_DEPTH` | LFO Amount | 0.0-1.0 | How much LFO affects volume (tremolo) |

### LFO (Low Frequency Oscillator)

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_LFO_RATE` | Rate | 0-20Hz | Speed of the LFO cycle |
| `FRUGI_LFO_MODE` | Mode | 0-1 | 0=Trigger (resets with each note), 1=Free-running |

### Performance Controls

| Parameter ID | Name | Range | Function |
|--------------|------|-------|----------|
| `FRUGI_PORTAMENTO` | Portamento | 0-1 | 0=Off, 1=On (enables glide between notes) |
| `FRUGI_PORTAMENTO_TIME` | Portamento Time | 0.0-1.0 | Glide time between notes |
| `FRUGI_PORTAMENTO_AMOUNT` | Portamento Amount | 0.0-1.0 | Character of the glide effect |
| `FRUGI_HOLD` | Hold Function | 0-1 | 0=Off, 1=On (sustains notes when enabled) |
| `FRUGI_MOD_WHEEL` | Mod Wheel Position | 0.0-1.0 | Current position of MIDI mod wheel |
| `FRUGI_CHORUS_MIX` | Chorus Mix | 0.0-1.0 | Amount of chorus effect |
