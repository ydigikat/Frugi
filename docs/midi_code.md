# Walking Through the MIDI Code (midi.c)
### Understanding MIDI Messages
MIDI is how keyboards, controllers, and sequencers talk to your synthesizer. 

The MIDI protocol sends messages as byte sequences, and our MIDI code interprets them.

### The MIDI Buffer
First, we need a place to store incoming MIDI bytes until we can process them:
```c
struct midi_ring_buffer
{
  uint8_t buffer[MIDI_BUFFER_SIZE];
  size_t head;
  size_t tail;
};

static struct midi_ring_buffer midi_buffer = {{0}, 0, 0};
```

This is a circular buffer (also called a ring buffer) that efficiently stores MIDI bytes as they arrive, even if we can't process them immediately.

### Parsing MIDI Messages
The main MIDI parsing function examines each byte to construct complete MIDI messages:
```c
struct midi_msg *midi_parse(struct midi_port *midi_in, uint8_t byte)
{
  struct midi_msg *msg = NULL;

  msg = midi_parse_rt_msg(midi_in, byte);

  if (msg == NULL)
  {
    if (midi_parse_message(midi_in, byte))
    {
      msg = midi_in->msg;
      midi_in->msg = NULL;
    }
  }

  return msg;
}
```

This code first checks if it's a "real-time" message (like clock pulses), and if not, adds it to the current message being built. When a complete message is formed, it returns it.

### Handling Different Message Types
MIDI has many message types, and the parsing code needs to handle them all:

```c
static bool midi_parse_message(struct midi_port *midi_in, uint8_t byte)
{
  // Status bytes start with 1, data bytes start with 0
  if (IS_STATUS_BYTE(byte))
  {
    midi_in->running_status = byte;
    midi_in->third_byte_expected = false;

    if (byte == MIDI_STATUS_SYS_EX_START)
    {
      midi_in->sysex_active = true;
      return false;
    }

    // Handle single-byte messages
    if (IS_SINGLE_BYTE_MSG(byte))
    {
      midi_in->msg->data[0] = byte;
      midi_in->msg->len = 1;
      return true;
    }

    return false;
  }

  // Handle messages differently based on their type
  switch (running_status)
  {
  case MIDI_STATUS_NOTE_ON:
  case MIDI_STATUS_NOTE_OFF:
  case MIDI_STATUS_CONTROL_CHANGE:
  case MIDI_STATUS_PITCH_BEND:
  case MIDI_STATUS_POLY_PRESSURE:
    midi_in->third_byte_expected = true;
    msg->data[0] = running_status;
    msg->data[1] = byte;
    msg->len = 2;
    return false;
    
  // ... other cases
  }
}
```
The code tracks the message state and knows which messages need 1, 2, or 3 bytes. It also handles MIDI's "running status" feature, where multiple messages can use the same status byte.

### Reading and Writing MIDI Data
Two simple functions handle adding data to and reading data from the MIDI buffer:

```c
void midi_buffer_write(uint8_t byte)
{
  uint16_t nextHead = (midi_buffer.head + 1) % MIDI_BUFFER_SIZE;

  if (nextHead != midi_buffer.tail)
  {
    midi_buffer.buffer[midi_buffer.head] = byte;
    midi_buffer.head = nextHead;
  }
}

bool midi_buffer_read(uint8_t *data)
{
  if (midi_buffer.head != midi_buffer.tail)
  {
    *data = midi_buffer.buffer[midi_buffer.tail];
    midi_buffer.tail = (midi_buffer.tail + 1) % MIDI_BUFFER_SIZE;
    return true;
  }
  return false;
}
```
These functions use the circular buffer to store MIDI bytes as they arrive and retrieve them when the DAE is ready to process them.

### MIDI Utilities
The MIDI code also includes some handy utilities:

- A lookup table to convert MIDI note numbers to frequencies
- Functions to convert between MIDI values and attenuation levels

```c
float midi_to_attenuation(uint32_t midi_value)
{
  if (midi_value == 0)
    return 0;

  return ((float)midi_value * (float)midi_value) / (127.0f * 127.0f);
}
```

This makes it easy to work with MIDI control values in your synthesizer code.

### How It All Works Together

1. MIDI bytes arrive from your hardware and get stored in the buffer
2. The DAE wakes up to fill an audio buffer and processes any waiting MIDI
3. Complete MIDI messages are passed to your synth code
4. Parameter changes (like filter cutoff from MIDI CC) are updated
5. Your synth code generates the next block of audio samples
6. The DAE formats them and passes them to the hardware
7. The cycle repeats, creating a continuous stream of audio!

This architecture keeps everything running smoothly, allowing your synthesizer to respond to MIDI input with minimal latency while maintaining glitch-free audio output.