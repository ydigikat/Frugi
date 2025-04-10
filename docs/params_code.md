# Understanding the Parameter Store in DAE
The parameter store in DAE is a simple but powerful system for managing all the settings in your synthesizer. Let's walk through how it works.

### What Does the Parameter Store Do?
Think of the parameter store as a central database for all your synthesizer's knobs and sliders:

1. It keeps track of filter cutoffs, resonance, envelope times, oscillator settings, etc.
2. It notifies your synth when values change
3. It handles conversions between different value formats

### The Code: params.c

The parameter store uses a straightforward design:
```c
// The memory where parameter values are stored
static float *param_store = NULL;
static size_t capacity;

// Flag that tells DAE when parameters have changed
bool dae_param_changed = false;
```
The store is just an array of floating-point values, with each parameter assigned an ID number (index).

### Initialization
When DAE starts up, it creates the parameter store:

```c
void dae_param_init(void)
{
    param_store = pvPortMalloc(DAE_PARAM_ALLOC_SIZE * sizeof(float));
    if (!param_store)
    {
        return;
    }
    capacity = DAE_PARAM_ALLOC_SIZE;    

    memset(param_store, 0, sizeof(float) * DAE_PARAM_ALLOC_SIZE);
}
```

This allocates memory for 64 parameters (the default size) and initializes all values to zero.

### Setting Parameters
There are two main functions for setting parameters:

```c 
// Set using a normalized value (0.0 to 1.0)
void param_set(uint16_t id, float norm_value)
{ 
    param_store[id] = norm_value;
    dae_param_changed = true;
}

// Set using a MIDI value (0 to 127)
void param_set_midi(uint16_t id, uint8_t value)
{
    param_store[id] = (float)value / 127.0f;     
    dae_param_changed = true;
}
```

Notice how each function:

1. Stores the value in the array
2. Sets the dae_param_changed flag to true

This flag is what DAE checks to know when to call your synth's dae_update_parameters() function.

### Getting Parameters
Reading a parameter is even simpler:

```c
float param_get(uint16_t id)
{
    return param_store[id];
}
```

### Value Conversion
The DAE provides handy macros (in params.h) to convert normalized values into more useful ranges:

```c
// Convert to an integer in a specific range
#define PARAM_TO_INT(norm, min, max) (((int)((norm * ((max) - (min)) + 0.5f)) + (min)))

// Linear scaling between min and max
#define PARAM_TO_LINEAR(norm, min, max) (min + (max-min)*norm)

// Exponential scaling (good for frequencies)
#define PARAM_TO_EXP(norm,min,max) (min * powf(max/min,norm))

// Power curve scaling (for custom response curves)
#define PARAM_TO_POWER(norm, min, max,exp) (min+(max-min)*powf(norm,exp))
```

### How to Use the Parameter Store
In your synthesizer code, you'll typically:

1. **Define parameter IDs at the top of your file**:
```c
#define PARAM_FILTER_CUTOFF 0
#define PARAM_RESONANCE 1
#define PARAM_ATTACK 2
// etc.
```

2. **Set initial values in your dae_prepare_for_play() function**:
```c
void dae_prepare_for_play(float sample_rate, size_t block_size, uint8_t *midi_channel)
{
    param_set(PARAM_FILTER_CUTOFF, 0.5f);
    param_set(PARAM_RESONANCE, 0.3f);
    // etc.
}
```

3. **Update from MIDI in your ```dae_handle_midi()``` function**:
```c
void dae_handle_midi(struct midi_msg *msg)
{
    if (msg->data[0] == MIDI_STATUS_CONTROL_CHANGE)
    {
        switch(msg->data[1]) // CC number
        {
            case MIDI_CC_RESONANCE:
                param_set_midi(PARAM_RESONANCE, msg->data[2]);
                break;
            // etc.
        }
    }
}
```

4. **Use in audio processing by getting and converting values**:
```c
void dae_update_parameters()
{
    // Get normalized value
    float norm = param_get(PARAM_FILTER_CUTOFF);
    
    // Convert to frequency (20Hz to 20kHz)
    float cutoff_freq = PARAM_TO_EXP(norm, 20.0f, 20000.0f);
    
    // Use in your filter algorithm
    update_filter(cutoff_freq);
}
```

That's really all there is to it! 
