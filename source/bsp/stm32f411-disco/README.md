## Hardware Configuration

### Clock Setup
- System clock: 100MHz (using 12MHz HSE with PLL)
- APB1: 50MHz
- APB2: 100MHz
- I2S PLL configured based on requested sample rate

### Performance
- ART (Adaptive Real-Time) memory accelerator is enabled with prefetch and instruction/data caches


### Audio Interface
- I2S3 peripheral in master transmit mode (to onboard codec)
- CS43L22 codec
- 32-bit data format, Philips standard
- DMAx Streamx configured for circular buffer operation
- GPIO pins: MCK(), CK()), WS(), SDO()

### MIDI Interface
- USART1 in receive-only mode
- 31250 baud, 8N1 configuration
- RX pin: Pxxx

### Debug and Development
- User LEDs on:  
- User button on PA0 (with pull-up)
- Debug probe pins on Pxx-Pxx

